// #include <ntstatus.h>
#include "pmd_driver/communication.h"
#include <winternl.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstdlib>

#include <setupapi.h>
#include <cfgmgr32.h>

HANDLE OpenDriverDevice()
{
    HDEVINFO deviceInfoSet;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = NULL;
    DWORD requiredSize = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    // Get device interface set
    deviceInfoSet = SetupDiGetClassDevs(
        &DRIVER1_DEVICE_GUID,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
    );

    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        printf("SetupDiGetClassDevs failed: %d\n", GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    // Get the first device interface
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (!SetupDiEnumDeviceInterfaces(
        deviceInfoSet,
        NULL,
        &DRIVER1_DEVICE_GUID,
        0,
        &deviceInterfaceData
    )) {
        printf("SetupDiEnumDeviceInterfaces failed: %d\n", GetLastError());
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return INVALID_HANDLE_VALUE;
    }

    // Get required size
    SetupDiGetDeviceInterfaceDetail(
        deviceInfoSet,
        &deviceInterfaceData,
        NULL,
        0,
        &requiredSize,
        NULL
    );

    // Allocate buffer
    deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
    if (!deviceInterfaceDetailData) {
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return INVALID_HANDLE_VALUE;
    }

    deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    // Get device path
    if (!SetupDiGetDeviceInterfaceDetail(
        deviceInfoSet,
        &deviceInterfaceData,
        deviceInterfaceDetailData,
        requiredSize,
        NULL,
        NULL
    )) {
        printf("SetupDiGetDeviceInterfaceDetail failed: %d\n", GetLastError());
        free(deviceInterfaceDetailData);
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return INVALID_HANDLE_VALUE;
    }

    printf("Device path: %s\n", deviceInterfaceDetailData->DevicePath);

    // Open the device
    hDevice = CreateFile(
        deviceInterfaceDetailData->DevicePath,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL /*| FILE_FLAG_OVERLAPPED*/, // currently no async yet
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("CreateFile failed: %d\n", GetLastError());
    }

    free(deviceInterfaceDetailData);
    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    return hDevice;
}

void CloseDriverDevice(HANDLE hDevice)
{
    if (hDevice != INVALID_HANDLE_VALUE && hDevice != NULL) {
        CloseHandle(hDevice);
    }
}

bool GetBlockOperation(HANDLE hDevice, BLOCK_OPERATION_DTO* dto)
{
    DWORD bytesReturned;
    memset(dto, 0, sizeof(BLOCK_OPERATION_DTO));

    if (!DeviceIoControl(
        hDevice,
        IOCTL_GET_NEXT_BLOCK_OPERATION,
        NULL,
        0,
        dto,
        sizeof(BLOCK_OPERATION_DTO),
        &bytesReturned,
        NULL
    )) {
        printf("ERROR: IOCTL_GET_NEXT_BLOCK_OPERATION failed: %d\n", GetLastError());
        return false;
    }

    if (bytesReturned >= sizeof(BLOCK_OPERATION_DTO) && dto->Version == 1) {
        // Successfully received a BLOCK_OPERATION_DTO
        return true;
    } else {
        printf("ERROR: Incomplete BLOCK_OPERATION_DTO received.\n");
        return false;
    }
}
