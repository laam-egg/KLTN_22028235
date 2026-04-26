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
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
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

bool RegisterWithDriver(HANDLE hDevice)
{
    DWORD bytesReturned;
    SCAN_TASK_DTO handshakeTask = { 0 };

    printf("=== Starting Driver Registration ===\n");

    if (!DeviceIoControl(
        hDevice,
        IOCTL_REGISTER_SCANNER,
        NULL,
        0,
        NULL,
        0,
        &bytesReturned,
        NULL
    )) {
        printf("ERROR: IOCTL_REGISTER_SCANNER failed: %d\n", GetLastError());
        return false;
    }
    printf("  ✓ Registration IOCTL sent successfully\n");

    return true;
}

bool GetPendingTaskAsync(
    HANDLE hDevice,
    PENDING_SCAN_TASK* io
) {
    ZeroMemory(io, sizeof(*io));

    io->ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!io->ov.hEvent) {
        return false;
    }

    DWORD ignored;
    BOOL ok = DeviceIoControl(
        hDevice,
        IOCTL_GET_PENDING_EXECUTABLE,
        NULL, 0,
        &io->dto,
        sizeof(io->dto),
        &ignored,
        &io->ov
    );

    if (!ok && GetLastError() != ERROR_IO_PENDING) {
        CloseHandle(io->ov.hEvent);
        return false;
    }

    return true;
}

bool CompletePendingTask(
    HANDLE hDevice,
    PENDING_SCAN_TASK* io,
    SCAN_TASK_DTO* outTask
) {
    DWORD bytes = 0;

    if (!GetOverlappedResult(hDevice, &io->ov, &bytes, FALSE)) {
        return false;
    }

    if (bytes != sizeof(*outTask)) {
        return false;
    }

    *outTask = io->dto;

    CloseHandle(io->ov.hEvent);
    io->ov.hEvent = NULL;

    return true;
}

void CancelPendingTask(HANDLE hDevice, PENDING_SCAN_TASK* pPendingTask) {
    if (pPendingTask->ov.hEvent) {
        CancelIoEx(hDevice, &pPendingTask->ov);
        CloseHandle(pPendingTask->ov.hEvent);
        pPendingTask->ov.hEvent = NULL;
    }
}

bool SendVerdict(HANDLE hDevice, SCAN_VERDICT_DTO* pVerdict)
{
    if (!pVerdict) return false;

    DWORD bytesReturned;
    if (!DeviceIoControl(
        hDevice,
        IOCTL_POST_SCANNING_RESULT,
        pVerdict,
        sizeof(SCAN_VERDICT_DTO),
        NULL,
        0,
        &bytesReturned,
        NULL
    )) {
        printf("ERROR: IOCTL_POST_SCANNING_RESULT failed: %d\n", GetLastError());
        return false;
    }

    return true;
}
