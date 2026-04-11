#include <ntddk.h>
#include <wdf.h>
#include <windef.h>     // DWORD
#include <winnt.h>      // FILE_ID_128
#include <ntifs.h>      // FILE_INTERNAL_INFORMATION
#include <bcrypt.h>
#include "config.h"

////////////////////////////
///////// Globals //////////
////////////////////////////

WDFDEVICE g_Device = NULL;

////////////////////////////
///////// Macros ///////////
////////////////////////////

#define nop() ((void)0)

/**
 * Call, check for errors and fail fast if any
 */

#define SAFETY_BEGIN() NTSTATUS status = STATUS_SUCCESS
#define SAFETY_END(cleanupCode) EXIT: cleanupCode return status;
#define SAFETY_END_RETURNING_VOID(cleanupCode) EXIT: cleanupCode return;
#define NO_CLEANUP nop();
#define FAILED_PREVIOUSLY() (!NT_SUCCESS(status))

#define TRY(functionCall) { status = functionCall; if (!NT_SUCCESS(status)) goto EXIT; } nop()
#define TRY_EXCEPT(functionCall, exceptBlock) { status = functionCall; if (!NT_SUCCESS(status)) { exceptBlock goto EXIT; } } nop()
#define ASSERT_NOT_NULL(value) if (value == NULL) FAIL_FAST()

/**
 * Either the call indeed never fails, or
 * that the errors are undetectable, cause
 * a bug check immediately... and we can't
 * do anything about it!
 */
#define NEVER_FAIL(functionCall) functionCall
#define SUCCEED_FAST() { status = STATUS_SUCCESS; goto EXIT; } nop()
#define FAIL_FAST() FAIL_FAST_WITH_STATUS(STATUS_FAIL_FAST_EXCEPTION)
#define FAIL_FAST_WITH_STATUS(x) { status = x; goto EXIT; } nop()

#define _LOG(type, prefix, ...) KdPrintEx((DPFLTR_IHVDRIVER_ID, type, prefix __VA_ARGS__))
#define LOG_INFO(...)   _LOG(DPFLTR_INFO_LEVEL, "@@@ driver1: INFO: ", __VA_ARGS__)
#define LOG_ERROR(...)  _LOG(DPFLTR_ERROR_LEVEL, "@@@ driver1: ERROR: ", __VA_ARGS__)

///////////////////////////////////
/////// Other Declarations ////////
///////////////////////////////////

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;
EVT_WDF_WORKITEM EvtAttestWorkItem;

VOID RoutineProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);
VOID EnqueueAttestWorkItem(WDFDEVICE Device, HANDLE ProcessId, PUNICODE_STRING ImagePath);

NTSTATUS GetFileIdFromPath(PUNICODE_STRING FilePath, PFILE_ID_128 FileId, PULONG VolumeSerial);

///////// IOCTLS //////////

#define IOCTL_GET_NONCE_FOR_PID CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_ATTEST_RESULT     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_WRITE_ACCESS)

////// DEVICE CONTEXT ///////

typedef struct _DEVICE_CONTEXT {
    LIST_ENTRY PendingList;
    WDFWAITLOCK ListLock;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext);

////// WORK ITEM CONTEXT ///////

typedef struct _WORK_ITEM_CONTEXT {
    HANDLE ProcessId;
    FILE_ID_128 FileId;              // Unique file ID
    ULONG VolumeSerialNumber;        // Volume identifier
    BOOLEAN FileIdValid;             // Whether we got the file ID successfully
} WORK_ITEM_CONTEXT, * PWORK_ITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WORK_ITEM_CONTEXT, WorkItemGetContext);

////// ATTEST ENTRY ///////

typedef struct _ATTEST_ENTRY {
    HANDLE Pid;
    UCHAR Nonce[32];
    LARGE_INTEGER Timestamp;
    BOOLEAN Attested;
    FILE_ID_128 FileId;              // Store file ID instead of path
    ULONG VolumeSerialNumber;
    LIST_ENTRY Link;
} ATTEST_ENTRY, * PATTEST_ENTRY;

struct AttestReport {
    HANDLE pid;
    UINT64 timestamp; // epoch or FILETIME
    UINT8 verdict;    // 0 = allow, 1 = block
    FILE_ID_128 fileId; // optional if needed
    ULONG volSerial;
    ULONG payloadSize; // size of payload bytes
    // payload bytes follow (exact bytes that were hashed and signed)
    // then signature length and signature bytes follow
}

// ============================================================
// DriverEntry
// ============================================================
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    SAFETY_BEGIN();
    LOG_INFO("DriverEntry starting");
    WDF_DRIVER_CONFIG config;

    LOG_INFO("Initializing driver");
    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);
    config.EvtDriverUnload = DriverUnload;
    TRY(WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE
    ));

    LOG_INFO("DriverEntry finished successfully");
    SAFETY_END(NO_CLEANUP);
}

// ============================================================
// DeviceAdd — create device + SDDL + interface
// ============================================================
NTSTATUS EvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    //UNREFERENCED_PARAMETER(Driver);

    //NTSTATUS status;
    //WDFDEVICE device;
    //WDF_IO_QUEUE_CONFIG ioQueueConfig;
    //PDEVICE_CONTEXT devCtx;
    //DECLARE_CONST_UNICODE_STRING(sddl, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)"); // SYSTEM & Admins only

    //WdfDeviceInitAssignSDDLString(DeviceInit, &sddl);
    //WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);

    //status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
    //if (!NT_SUCCESS(status)) return status;

    //devCtx = DeviceGetContext(device);
    //InitializeListHead(&devCtx->PendingList);
    //WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &devCtx->ListLock);

    //static const GUID MyDeviceGUID = { 0xa1b2c3d4, 0x1111, 0x2222, {0x33,0x44,0x55,0x66,0x77,0x88,0x99,0x00} };
    //WdfDeviceCreateDeviceInterface(device, &MyDeviceGUID, NULL);

    //WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchSequential);
    //ioQueueConfig.EvtIoDeviceControl = EvtIoDeviceControl;
    //WdfIoQueueCreate(device, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);

    //// Register for process creation notifications
    //PsSetCreateProcessNotifyRoutineEx(MyProcessNotify, FALSE);

    //return STATUS_SUCCESS;

    LOG_INFO("EvtDeviceAdd starting");

    SAFETY_BEGIN();

    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDFQUEUE queue;
    PDEVICE_CONTEXT devCtx;
    DECLARE_CONST_UNICODE_STRING(sddl, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)"); // SYSTEM & Admins only

    LOG_INFO("Creating device");
    //WDF_OBJECT_ATTRIBUTES_INIT(&deviceAttributes);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);
    WdfDeviceInitAssignSDDLString(DeviceInit, &sddl);
    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);
    TRY(WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device));
    g_Device = device;

    LOG_INFO("Creating device context");
    devCtx = DeviceGetContext(device);
    InitializeListHead(&devCtx->PendingList); // initialize a doubly-linked list
    TRY(WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &devCtx->ListLock));

    LOG_INFO("Assigning GUID to device");
    TRY(WdfDeviceCreateDeviceInterface(device, &DRIVER1_DEVICE_GUID, NULL));

    LOG_INFO("Creating default I/O queue for device");
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential /*or WdfIoQueueDispatchParallel*/);
    queueConfig.EvtIoDeviceControl = EvtIoDeviceControl;
    TRY(WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue));

    LOG_INFO("Registering process notification callback");
    TRY(PsSetCreateProcessNotifyRoutineEx(RoutineProcessNotify, /*Remove=*/FALSE));

    LOG_INFO("EvtDeviceAdd finished successfully");
    SAFETY_END({
        if (FAILED_PREVIOUSLY()) {
            PsSetCreateProcessNotifyRoutineEx(RoutineProcessNotify, /*Remove=*/TRUE);
        }
    });
}

// ============================================================
// Process creation callback
// ============================================================
VOID RoutineProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
    UNREFERENCED_PARAMETER(Process);

    SAFETY_BEGIN();
    LOG_INFO("RoutingProcessNotify starting");
    LOG_INFO("Process ID: %p", ProcessId);

    if (CreateInfo == NULL) {
        SUCCEED_FAST(); // process terminating
    }

    // Defer work to PASSIVE_LEVEL
    NEVER_FAIL(EnqueueAttestWorkItem(g_Device, ProcessId, CreateInfo->ImageFileName));

    LOG_INFO("RoutingProcessNotify finished successfully");
    SAFETY_END_RETURNING_VOID(NO_CLEANUP);
}

// ============================================================
// Queue a work item for attestation
// ============================================================
VOID EnqueueAttestWorkItem(WDFDEVICE Device, HANDLE ProcessId, PUNICODE_STRING ImagePath)
{
    SAFETY_BEGIN();
    LOG_INFO("EnqueueAttestWorkItem starting");

    WDF_OBJECT_ATTRIBUTES attr;
    WDF_WORKITEM_CONFIG cfg;
    WDFWORKITEM workItem;
    PWORK_ITEM_CONTEXT wiCtx = NULL;

    LOG_INFO("Configuring work item with context");
    WDF_WORKITEM_CONFIG_INIT(&cfg, EvtAttestWorkItem);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, WORK_ITEM_CONTEXT);
    attr.ParentObject = Device;

    LOG_INFO("Creating work item");
    TRY(WdfWorkItemCreate(&cfg, &attr, &workItem));

    {
        // Store context data
        wiCtx = WorkItemGetContext(workItem);
        ASSERT_NOT_NULL(wiCtx);
        wiCtx->ProcessId = ProcessId;
        wiCtx->FileIdValid = FALSE;

        // Try to get file ID
        if (ImagePath && ImagePath->Length > 0 && ImagePath->Buffer) {
            status = GetFileIdFromPath(
                ImagePath,
                &wiCtx->FileId,
                &wiCtx->VolumeSerialNumber
            );

            if (NT_SUCCESS(status)) {
                wiCtx->FileIdValid = TRUE;
                LOG_INFO("Got file ID for process %p", ProcessId);
            }
            else {
                LOG_INFO(
                    "Failed to get file ID for process %p: status = 0x%08X | Blocking this guy's execution.",
                    ProcessId, status
                );
                // TODO: BLOCK EXECUTION
            }
        }
    }

    NEVER_FAIL(WdfWorkItemEnqueue(workItem));

    LOG_INFO("EnqueueAttestWorkItem finished successfully");
    SAFETY_END_RETURNING_VOID(NO_CLEANUP);
}

// ============================================================
// Work item — generate nonce and record pending entry
// ============================================================
VOID EvtAttestWorkItem(WDFWORKITEM WorkItem)
{
    SAFETY_BEGIN();
    LOG_INFO("EvtAttestWorkItem starting");

    PATTEST_ENTRY attestEntry = NULL;
    UCHAR nonce[32];

    LOG_INFO("Getting the WDFDEVICE that owns this work item");
    WDFDEVICE device = NEVER_FAIL(WdfWorkItemGetParentObject(WorkItem));

    LOG_INFO("Getting device context");
    PDEVICE_CONTEXT ctx = NEVER_FAIL(DeviceGetContext(device));

    LOG_INFO("Retrieve the workitem context");
    PWORK_ITEM_CONTEXT wiCtx = WorkItemGetContext(WorkItem);
    if (!wiCtx) {
        LOG_ERROR("No work-item context");
        FAIL_FAST();
    }

    LOG_INFO("Generating nonce for usermode component attestation");
    TRY(BCryptGenRandom(NULL, nonce, sizeof(nonce), BCRYPT_USE_SYSTEM_PREFERRED_RNG));

    LOG_INFO("Allocating attestation entry in non-paged pool");
    attestEntry = (PATTEST_ENTRY)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(ATTEST_ENTRY), 'tstA');
    ASSERT_NOT_NULL(attestEntry);

    LOG_INFO("Initializing attestion entry");
    NEVER_FAIL(RtlZeroMemory(attestEntry, sizeof(*attestEntry)));
    NEVER_FAIL(RtlCopyMemory(attestEntry->Nonce, nonce, sizeof(nonce)));
    attestEntry->Pid = wiCtx->ProcessId;
    attestEntry->Attested = FALSE;
    attestEntry->FileId = wiCtx->FileId;
    attestEntry->VolumeSerialNumber = wiCtx->VolumeSerialNumber;
    // attestEntry->FileIdValid = wiCtx->FileIdValid; // for invalid file ID, block downright
    NEVER_FAIL(KeQuerySystemTime(&attestEntry->Timestamp));

    LOG_INFO("Acquiring lock to send attest entry into queue");
    TRY(WdfWaitLockAcquire(ctx->ListLock, NULL));
    NEVER_FAIL(InsertTailList(&ctx->PendingList, &attestEntry->Link));
    NEVER_FAIL(WdfWaitLockRelease(ctx->ListLock));

    // At this point, user-mode component can query attestation entry via IOCTL
    LOG_INFO("Nonce generated and pending entry inserted for PID %p", attestEntry->Pid);
    LOG_INFO("EvtAttestWorkItem finished successfully");

    SAFETY_END_RETURNING_VOID({
        if (FAILED_PREVIOUSLY()) {
            if (attestEntry != NULL) {
                ExFreePoolWithTag(attestEntry, 'tstA');
            }
        }
        WdfObjectDelete(WorkItem);
    });
}

// ============================================================
// IOCTL handler — verifier communication
// ============================================================
VOID EvtIoDeviceControl(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
) {
    SAFETY_BEGIN();
    LOG_INFO("EvtIoDeviceControl starting");

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PDEVICE_CONTEXT ctx = DeviceGetContext(device);

    switch (IoControlCode) {
    case IOCTL_GET_NONCE_FOR_PID:
        // TODO: find entry by PID and return nonce
        break;

    case IOCTL_ATTEST_RESULT:
        // TODO: verify PID, mark attested, or terminate process
        break;

    default:
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
        return;
    }

    WdfRequestComplete(Request, STATUS_SUCCESS);

    LOG_INFO("EvtIoDeviceControl finished successfully");
    SAFETY_END_RETURNING_VOID(NO_CLEANUP);
}














VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    if (g_Device) {
        // TODO: How to be sure that all requests have been handled BEFORE unloading like this?
        PDEVICE_CONTEXT ctx = DeviceGetContext(g_Device);
        if (ctx) {
            WdfWaitLockAcquire(ctx->ListLock, NULL);
            while (!IsListEmpty(&ctx->PendingList)) {
                PLIST_ENTRY entry = RemoveHeadList(&ctx->PendingList);
                PATTEST_ENTRY e = CONTAINING_RECORD(entry, ATTEST_ENTRY, Link);
                ExFreePoolWithTag(e, 'tstA');
            }
            WdfWaitLockRelease(ctx->ListLock);
        }
    }
    PsSetCreateProcessNotifyRoutineEx(RoutineProcessNotify, TRUE);
}

// TODO: Review this

NTSTATUS GetFileIdFromPath(
    PUNICODE_STRING FilePath,
    PFILE_ID_128 FileId,
    PULONG VolumeSerial
) {
    NTSTATUS status;
    HANDLE fileHandle = NULL;
    OBJECT_ATTRIBUTES objAttr;
    IO_STATUS_BLOCK iosb;
    PVOID buffer = NULL;

    if (!FilePath || !FilePath->Buffer) return STATUS_INVALID_PARAMETER;

    InitializeObjectAttributes(
        &objAttr,
        FilePath,
        OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    status = ZwCreateFile(
        &fileHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &objAttr,
        &iosb,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Query FileIdInformation
    #define BUFFER_LENGTH sizeof(FILE_ID_INFORMATION)
    buffer = ExAllocatePoolWithTag(NonPagedPoolNx, BUFFER_LENGTH, 'fid ');
    if (!buffer) {
        ZwClose(fileHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(buffer, BUFFER_LENGTH);
    #undef BUFFER_LENGTH

    status = ZwQueryInformationFile(fileHandle, &iosb, buffer, sizeof(FILE_ID_INFORMATION), FileIdInformation);
    if (NT_SUCCESS(status)) {
        PFILE_ID_INFORMATION fidInfo = (PFILE_ID_INFORMATION)buffer;
        RtlCopyMemory(FileId, &fidInfo->FileId, sizeof(FILE_ID_128));
    }
    ExFreePoolWithTag(buffer, 'fid ');

    // Query Volume serial - allocate a larger buffer for FILE_FS_VOLUME_INFORMATION
    if (NT_SUCCESS(status)) {
        ULONG volBufLen = sizeof(FILE_FS_VOLUME_INFORMATION) + 1024;
        buffer = ExAllocatePoolWithTag(NonPagedPoolNx, volBufLen, 'vol ');
        if (!buffer) {
            ZwClose(fileHandle);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(buffer, volBufLen);

        status = ZwQueryVolumeInformationFile(fileHandle, &iosb, buffer, volBufLen, FileFsVolumeInformation);
        if (NT_SUCCESS(status)) {
            PFILE_FS_VOLUME_INFORMATION volInfo = (PFILE_FS_VOLUME_INFORMATION)buffer;
            *VolumeSerial = volInfo->VolumeSerialNumber;
        }
        ExFreePoolWithTag(buffer, 'vol ');
    }

    ZwClose(fileHandle);
    return status;
}
