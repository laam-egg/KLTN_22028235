// TODO: Differentiate between usermode component
// executable/process from the rest!

#define DRIVER1_KERNEL_MODE

// Include kernel headers in the correct order FIRST
#include <ntddk.h>
#include <wdf.h>
#include <bcrypt.h>
#include "config.h"

// Copied from <ntifs.h>
typedef struct _FILE_ID_INFORMATION {
    ULONGLONG VolumeSerialNumber;
    FILE_ID_128 FileId;
} FILE_ID_INFORMATION, * PFILE_ID_INFORMATION;







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

#define SAFETY_BEGIN() NTSTATUS status = STATUS_SUCCESS; (void)status;
#define WILL_USE_LOCK(Name) BOOLEAN lock##Name##Held = FALSE
#define SAFETY_END(cleanupCode) EXIT: cleanupCode return status;
#define SAFETY_END_RETURNING_VOID(cleanupCode) goto EXIT; EXIT: cleanupCode return;
#define NO_CLEANUP nop();
#define FAILED_PREVIOUSLY() (!NT_SUCCESS(status))

#define TRY(functionCall) { status = functionCall; if (!NT_SUCCESS(status)) goto EXIT; } nop()
#define TRY_EXCEPT(functionCall, exceptBlock) { status = functionCall; if (!NT_SUCCESS(status)) { exceptBlock goto EXIT; } } nop()
#define ASSERT_NOT_NULL(value) if (value == NULL) FAIL_FAST()

#define WAITLOCK_ACQUIRE(Name, lockHandle, timeout) { TRY(WdfWaitLockAcquire(lockHandle, timeout)); lock##Name##Held = TRUE; } nop()
#define WAITLOCK_RELEASE(Name, lockHandle)          { NEVER_FAIL(WdfWaitLockRelease(lockHandle)); lock##Name##Held = FALSE; } nop()
#define WAITLOCK_CLEANUP(Name, lockHandle)          if (lock##Name##Held) { WAITLOCK_RELEASE(Name, lockHandle); } nop()

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
EVT_WDF_DRIVER_UNLOAD DriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;
EVT_WDF_WORKITEM EvtAddScanTaskToPendingList;

VOID RoutineProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);

VOID EnqueueScanTaskAddWorkItem(WDFDEVICE Device, HANDLE ProcessId, PCUNICODE_STRING ImagePath);

NTSTATUS GetFileIdFromPath(PCUNICODE_STRING FilePath, PFILE_ID_128 FileId, PULONG VolumeSerial);

////// DEVICE CONTEXT ///////

typedef struct _DEVICE_CONTEXT {
    /**
     * List of executables that are waiting to be pulled off
     * and scanned by the usermode component. Once an entry
     * in this list is popped off, it will be forwarded to
     * usermode component via IOCTL and also persisted in
     * ScanningList (below).
     */
    LIST_ENTRY PendingList;

    /**
     * Lock for PendingList. Use in PASSIVE_LEVEL only.
     */
    WDFWAITLOCK PendingListLock;

    /**
     * List of executables that are already being scanned.
     * In case the usermode component FAILS to attest/scan,
     * the corresponding entry in this list is moved back to
     * PendingList.
     */
    LIST_ENTRY ScanningList;

    /**
     * Lock for ScanningList. Use in PASSIVE_LEVEL only.
     */
    WDFWAITLOCK ScanningListLock;
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

////// SCAN TASK (AN ELEMENT IN PendingList or ScanningList OF DEVICE CONTEXT) //////////

typedef struct _SCAN_TASK {
    SCAN_TASK_DTO Dto;
    LIST_ENTRY Link;
} SCAN_TASK, * PSCAN_TASK;

//////////// IOCTL HANDLERS (CONTEXT-DEPENDENT) /////////////

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLGetPendingExecutable(
    WDFDEVICE device,
    PDEVICE_CONTEXT ctx,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength
);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLPostScanningResult(
    WDFDEVICE device,
    PDEVICE_CONTEXT ctx,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength
);

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
    UNREFERENCED_PARAMETER(Driver);

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
    InitializeListHead(&devCtx->PendingList);
    TRY(WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &devCtx->PendingListLock));
    InitializeListHead(&devCtx->ScanningList);
    TRY(WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &devCtx->ScanningListLock));

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
        // TODO: Clean up for WdfIoQueueCreate, WdfWaitLockCreate x2
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
    NEVER_FAIL(EnqueueScanTaskAddWorkItem(g_Device, ProcessId, CreateInfo->ImageFileName));

    LOG_INFO("RoutingProcessNotify finished successfully");
    SAFETY_END_RETURNING_VOID(NO_CLEANUP);
}

// ============================================================
// Queue a pending executable for scan
// ============================================================
VOID EnqueueScanTaskAddWorkItem(WDFDEVICE Device, HANDLE ProcessId, PCUNICODE_STRING ImagePath)
{
    SAFETY_BEGIN();
    LOG_INFO("EnqueueScanTaskAddWorkItem starting");

    WDF_OBJECT_ATTRIBUTES attr;
    WDF_WORKITEM_CONFIG cfg;
    WDFWORKITEM workItem;
    PWORK_ITEM_CONTEXT wiCtx = NULL;

    LOG_INFO("Configuring work item with context");
    WDF_WORKITEM_CONFIG_INIT(&cfg, EvtAddScanTaskToPendingList);
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

    LOG_INFO("EnqueueScanTaskAddWorkItem finished successfully");
    SAFETY_END_RETURNING_VOID(NO_CLEANUP);
}

// ============================================================
// Adds new scan task to PendingList
// ============================================================
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID EvtAddScanTaskToPendingList(WDFWORKITEM WorkItem)
{
    SAFETY_BEGIN();
    WILL_USE_LOCK(LPENDING);
    
    LOG_INFO("EvtAddScanTaskToPendingList starting");

    PSCAN_TASK pScanTask = NULL;
    BYTE nonce[32];
    WDFDEVICE device = NULL;
    PDEVICE_CONTEXT ctx = NULL;
    PWORK_ITEM_CONTEXT wiCtx = NULL;

    LOG_INFO("Getting the WDFDEVICE that owns this work item");
    device = NEVER_FAIL(WdfWorkItemGetParentObject(WorkItem));

    LOG_INFO("Getting device context");
    ctx = NEVER_FAIL(DeviceGetContext(device));

    LOG_INFO("Retrieve the workitem context");
    wiCtx = WorkItemGetContext(WorkItem);
    if (!wiCtx) {
        LOG_ERROR("No work-item context");
        FAIL_FAST();
    }

    LOG_INFO("Generating nonce for usermode component attestation");
    TRY(BCryptGenRandom(NULL, nonce, sizeof(nonce), BCRYPT_USE_SYSTEM_PREFERRED_RNG));

    LOG_INFO("Allocating attestation entry in non-paged pool");
    #pragma warning(disable: 4996)
    pScanTask = (PSCAN_TASK)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(SCAN_TASK), 'tstA');
    ASSERT_NOT_NULL(pScanTask);

    LOG_INFO("Initializing attestion entry");
    NEVER_FAIL(RtlZeroMemory(pScanTask, sizeof(SCAN_TASK)));
    NEVER_FAIL(RtlCopyMemory(pScanTask->Dto.Nonce, nonce, sizeof(nonce)));
    pScanTask->Dto.Pid = wiCtx->ProcessId;
    NEVER_FAIL(KeQuerySystemTime(&pScanTask->Dto.Timestamp));
    pScanTask->Dto.FileId = wiCtx->FileId;
    pScanTask->Dto.VolumeSerialNumber = wiCtx->VolumeSerialNumber;

    LOG_INFO("Acquiring lock to send attest entry into queue");
    WAITLOCK_ACQUIRE(LPENDING, ctx->PendingListLock, NULL);
    NEVER_FAIL(InsertTailList(&ctx->PendingList, &pScanTask->Link));
    WAITLOCK_RELEASE(LPENDING, ctx->PendingListLock);

    // At this point, user-mode component can query scan task via IOCTL
    LOG_INFO("Nonce generated and pending entry inserted for PID %p", pScanTask->Dto.Pid);
    LOG_INFO("EvtAddScanTaskToPendingList finished successfully");

    SAFETY_END_RETURNING_VOID({
        if (FAILED_PREVIOUSLY()) {
            if (pScanTask != NULL) {
                ExFreePoolWithTag(pScanTask, 'tstA');
            }
        }
        WAITLOCK_CLEANUP(LPENDING, ctx->PendingListLock);
        WdfObjectDelete(WorkItem);
    });
}

// ============================================================
// IOCTL handler
// ============================================================
_IRQL_requires_max_(PASSIVE_LEVEL)
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
    case IOCTL_GET_PENDING_EXECUTABLE:
        HandleIOCTLGetPendingExecutable(device, ctx, Request, OutputBufferLength, InputBufferLength);
        break;

    case IOCTL_POST_SCANNING_RESULT:
        HandleIOCTLPostScanningResult(device, ctx, Request, OutputBufferLength, InputBufferLength);
        break;

    default:
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
        return;
    }

    WdfRequestComplete(Request, STATUS_SUCCESS);

    LOG_INFO("EvtIoDeviceControl finished successfully");
    SAFETY_END_RETURNING_VOID(NO_CLEANUP);
}

/**
 * Pops off and forwards (to usermode) the next
 * scan task in PendingList.
 * 
 * Also moves this task to ScanningList.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLGetPendingExecutable(
    WDFDEVICE device,
    PDEVICE_CONTEXT ctx,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength
) {
    UNREFERENCED_PARAMETER(device);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    SAFETY_BEGIN();
    WILL_USE_LOCK(LPending);

    WAITLOCK_ACQUIRE(LPending, ctx->PendingListLock, NULL);

    if (!IsListEmpty(&ctx->PendingList)) {
        // Send a zeroed-out SCAN_TASK to usermode, or NULL if possible
        SUCCEED_FAST();
    }

    PLIST_ENTRY entry = RemoveHeadList(&ctx->PendingList);
    PSCAN_TASK pScanTask = CONTAINING_RECORD(entry, SCAN_TASK, Link);
    (void)pScanTask;

    // Send this SCAN_TASK to usermode

    // TODO: Should we release this lock earlier?
    WAITLOCK_RELEASE(LPending, ctx->PendingListLock);

    SAFETY_END_RETURNING_VOID({
        WAITLOCK_CLEANUP(LPending, ctx->PendingListLock);
    });
}

/**
 * Gets the scanning result from usermode
 * component, check attestation and take
 * action.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLPostScanningResult(
    WDFDEVICE device,
    PDEVICE_CONTEXT ctx,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength
) {
    // TODO
    UNREFERENCED_PARAMETER(device);
    UNREFERENCED_PARAMETER(ctx);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);
}












_IRQL_requires_max_(PASSIVE_LEVEL)
VOID DriverUnload(WDFDRIVER driver)
{
    UNREFERENCED_PARAMETER(driver);
    SAFETY_BEGIN();
    WILL_USE_LOCK(LPending);
    WILL_USE_LOCK(LScanning);

    PDEVICE_CONTEXT ctx = NULL;

    if (g_Device) {
        ctx = DeviceGetContext(g_Device);
        if (ctx) {
            WAITLOCK_ACQUIRE(LPending, ctx->PendingListLock, NULL);
            while (!IsListEmpty(&ctx->PendingList)) {
                PLIST_ENTRY entry = RemoveHeadList(&ctx->PendingList);
                ASSERT_NOT_NULL(entry);
                PSCAN_TASK pScanTask = CONTAINING_RECORD(entry, SCAN_TASK, Link);
                ASSERT_NOT_NULL(pScanTask);
                ExFreePoolWithTag(pScanTask, 'tstA');
            }
            WAITLOCK_RELEASE(LPending, ctx->PendingListLock);

            WAITLOCK_ACQUIRE(LScanning, ctx->ScanningListLock, NULL);
            while (!IsListEmpty(&ctx->ScanningList)) {
                PLIST_ENTRY entry = RemoveHeadList(&ctx->ScanningList);
                ASSERT_NOT_NULL(entry);
                PSCAN_TASK pScanTask = CONTAINING_RECORD(entry, SCAN_TASK, Link);
                ASSERT_NOT_NULL(pScanTask);
                ExFreePoolWithTag(pScanTask, 'tstA');
            }
            WAITLOCK_RELEASE(LScanning, ctx->ScanningListLock);
        }
    }
    PsSetCreateProcessNotifyRoutineEx(RoutineProcessNotify, TRUE);

    SAFETY_END_RETURNING_VOID({
        WAITLOCK_CLEANUP(LPending, ctx->PendingListLock);
        WAITLOCK_CLEANUP(LScanning, ctx->ScanningListLock);
    });
}








// TODO: Review this

NTSTATUS GetFileIdFromPath(PCUNICODE_STRING FilePath, PFILE_ID_128 FileId, PULONG VolumeSerial) {
    NTSTATUS status;
    HANDLE fileHandle = NULL;
    OBJECT_ATTRIBUTES objAttr;
    IO_STATUS_BLOCK iosb;
    PVOID buffer = NULL;

    if (!FilePath || !FilePath->Buffer) return STATUS_INVALID_PARAMETER;

    #pragma warning(push)
    #pragma warning(disable: 4090)  // Different 'const' qualifiers
    InitializeObjectAttributes(
        &objAttr,
        FilePath,
        OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );
    #pragma warning(pop)

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
    #pragma warning(disable: 4996)
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
        #pragma warning(disable: 4996)
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
