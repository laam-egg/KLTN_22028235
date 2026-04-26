#define DRIVER1_KERNEL_MODE

// Define WDF version BEFORE including headers
// https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/kmdf-version-history

#ifndef KMDF_VERSION_MAJOR
#define KMDF_VERSION_MAJOR 1
#endif

#ifndef KMDF_VERSION_MINOR
#define KMDF_VERSION_MINOR 15
#endif

#if (KMDF_VERSION_MAJOR != 1 || KMDF_VERSION_MINOR < 15)
#error Code only compiles on KMDF 1.15+ (Windows 8.1+)
#endif

// Include kernel headers in the correct order FIRST
#include <ntddk.h>
#include <wdf.h>
#include <wdfrequest.h>
#include <bcrypt.h>
#include "config.h"
#include "proctools.h"
#include "drivertools.h"

// Copied from <ntifs.h>
typedef struct _FILE_ID_INFORMATION {
    ULONGLONG VolumeSerialNumber;
    FILE_ID_128 FileId;
} FILE_ID_INFORMATION, * PFILE_ID_INFORMATION;







////////////////////////////
///////// Globals //////////
////////////////////////////

WDFDEVICE g_Device = NULL;

typedef struct _TRUSTED_PARTNER {
    HANDLE Pid;
    KSPIN_LOCK Lock;
} TRUSTED_PARTNER;

TRUSTED_PARTNER g_Service = { 0 };

////////////////////////////
///////// Macros ///////////
////////////////////////////

#define nop() ((void)0)

/**
 * Call, check for errors and fail fast if any
 */

#define SAFETY_BEGIN() NTSTATUS status = STATUS_SUCCESS; (void)status;
#define SAFETY_END(cleanupCode) EXIT: cleanupCode return status;
#define SAFETY_END_RETURNING_VOID(cleanupCode) goto EXIT; EXIT: cleanupCode return;
#define NO_CLEANUP nop();
#define FAILED_PREVIOUSLY() (!NT_SUCCESS(status))
#define FAILED_PREVIOUSLY_WITH(code) (status == (code))

#define TRY(functionCall) { status = functionCall; if (!NT_SUCCESS(status)) { LOG_ERROR("At file %s, line %d: status = %lu", __FILE__, __LINE__, status); goto EXIT; } } nop()
#define TRY_EXCEPT(functionCall, exceptBlock) { status = functionCall; if (!NT_SUCCESS(status)) { exceptBlock goto EXIT; } } nop()
#define ASSERT_NOT_NULL(value) if (value == NULL) FAIL_FAST()

#define WILL_USE_LOCK(Name) BOOLEAN lock##Name##Held = FALSE
#define WAITLOCK_ACQUIRE(Name, lockHandle, timeout) { TRY(WdfWaitLockAcquire(lockHandle, timeout)); lock##Name##Held = TRUE; } nop()
#define WAITLOCK_RELEASE(Name, lockHandle)          { NEVER_FAIL(WdfWaitLockRelease(lockHandle)); lock##Name##Held = FALSE; } nop()
#define WAITLOCK_CLEANUP(Name, lockHandle)          if (lock##Name##Held) { WAITLOCK_RELEASE(Name, lockHandle); } nop()
#define SPINLOCK_ACQUIRE(Name, lockHandle)      { NEVER_FAIL(WdfSpinLockAcquire(lockHandle)); lock##Name##Held = TRUE; } nop()
#define SPINLOCK_RELEASE(Name, lockHandle)      { NEVER_FAIL(WdfSpinLockRelease(lockHandle)); lock##Name##Held = FALSE; } nop()
#define SPINLOCK_CLEANUP(Name, lockHandle)      if (lock##Name##Held) { SPINLOCK_RELEASE(Name, lockHandle); } nop()

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

#define _LOG(type, prefix, ...) { KdPrintEx((DPFLTR_IHVDRIVER_ID, type, prefix __VA_ARGS__)); KdPrintEx((DPFLTR_IHVDRIVER_ID, type, "\n")); } nop()
#define LOG_INFO(...)   _LOG(DPFLTR_INFO_LEVEL, "@@@ driver1: INFO: ", __VA_ARGS__)
#define LOG_ERROR(...)  _LOG(DPFLTR_ERROR_LEVEL, "@@@ driver1: ERROR: ", __VA_ARGS__)
#define LOG_WARN(...)   _LOG(DPFLTR_WARNING_LEVEL, "@@@ driver1: WARNING: ", __VA_ARGS__)

inline HANDLE GET_CALLER_ID(WDFREQUEST Request) {
    PIRP irp = WdfRequestWdmGetIrp(Request);
    return GetCallerIDFromIRP(irp);
}

#define MAX_RETRIES 5
#define TAG_BLOCK_OPERATION 'blko'

#define CLEANUP_ALL_SCAN_TASKS 1

///////////////////////////////////
/////// Other Declarations ////////
///////////////////////////////////

DRIVER_INITIALIZE DriverEntry;

_Function_class_(EVT_WDF_DRIVER_UNLOAD)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
EVT_WDF_DRIVER_UNLOAD DriverUnload;

_Function_class_(EVT_WDF_DRIVER_DEVICE_ADD)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL) // The rule: IRQL annotations describe the required entry IRQL ; NOT the max IRQL that this function itself could raise upto
EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;

_Function_class_(EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;

_Function_class_(EVT_WDF_WORKITEM)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
EVT_WDF_WORKITEM EvtAddScanTaskToPendingList;

_Function_class_(EVT_WDF_WORKITEM)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
EVT_WDF_WORKITEM EvtConductCleanupTask;

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID RoutineProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
static VOID EnqueueScanTaskAddWorkItem(WDFDEVICE Device, HANDLE ProcessId, PCUNICODE_STRING ImagePath);

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
static VOID EnqueueCleanupTaskWorkItem(WDFDEVICE Device, INT CleanupTask, PVOID Arg);



_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
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
     * Lock for PendingList. It's a SPIN LOCK.
     */
    WDFSPINLOCK PendingListSpinLock;

    /**
     * List of GetNextScanTask IOCTL requests.
     * Since we are doing async scanning,
     * whenever the Service yields that IOCTL,
     * we save the WDFREQUEST into this queue.
     * And when the PendingList is filled,
     * we pops off the head of this queue to get
     * the next WDFREQUEST to fulfill.
     * 
     * This is akin to HTTP long-polling, while
     * the previous strategy is akin to HTTP
     * short-polling, which wastes CPU time
     * when the Service would have to poll
     * the driver at fixed interval to get
     * the next PE file to scan, or none at all.
     */
    WDFQUEUE ScanRequestQueue;

    /**
     * List of past decisions made by this driver i.e. allow/deny
     * execution, of which files, when... Mean to be read via
     * IOCTL_GET_NEXT_BLOCK_OPERATION.
     */
    LIST_ENTRY BlockOperationList;

    /**
     * Lock for BlockOperationList. Use in PASSIVE_LEVEL only.
     */
    WDFWAITLOCK BlockOperationListLock;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext);

////// WORK ITEM CONTEXT ///////

typedef struct _WORK_ITEM_CONTEXT {
    HANDLE ProcessId;
    FILE_ID_128 FileId;              // Unique file ID
    ULONG VolumeSerialNumber;        // Volume identifier
} WORK_ITEM_CONTEXT, * PWORK_ITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WORK_ITEM_CONTEXT, WorkItemGetContext);

////// CLEANUP ITEM CONTEXT ///////

typedef struct _CLEANUP_ITEM_CONTEXT {
    INT CleanupTask;
} CLEANUP_ITEM_CONTEXT, *PCLEANUP_ITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CLEANUP_ITEM_CONTEXT, CleanupItemGetContext);

////// SCAN REQUEST (AN ELEMENT IN ScanRequestQueue OF DEVICE CONTEXT) //////////
typedef struct _SCAN_REQUEST {
    WDFREQUEST Request;
} SCAN_REQUEST, *PSCAN_REQUEST;

////// SCAN TASK (AN ELEMENT IN PendingList OF DEVICE CONTEXT) //////////

typedef struct _SCAN_TASK {
    SCAN_TASK_DTO Dto;
    LIST_ENTRY Link;
    HANDLE CallerPid;
    INT8 Retried;
} SCAN_TASK, *PSCAN_TASK;

////// BLOCK OPERATION (AN ELEMENT IN BlockOperationList OF DEVICE CONTEXT) //////////

typedef struct _BLOCK_OPERATION {
    BLOCK_OPERATION_DTO Dto;
    LIST_ENTRY Link;
} BLOCK_OPERATION, *PBLOCK_OPERATION;

//////////// IOCTL HANDLERS (CONTEXT-DEPENDENT) /////////////

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLRegisterScanner(
    WDFDEVICE device,
    PDEVICE_CONTEXT ctx,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength
);

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLGetPendingExecutable(
    WDFDEVICE device,
    PDEVICE_CONTEXT ctx,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength
);

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLPostScanningResult(
    WDFDEVICE device,
    PDEVICE_CONTEXT ctx,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength
);

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLGetNextBlockOperation(
    WDFDEVICE device,
    PDEVICE_CONTEXT ctx,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength
);






_IRQL_requires_max_(DISPATCH_LEVEL)
VOID CompleteScanRequest(WDFREQUEST Request, PSCAN_TASK pScanTask);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID EmitterTryingToCompleteScanRequest(PSCAN_TASK pScanTask);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID ConsumerTryingToCompleteScanRequest(WDFREQUEST Request);





// ============================================================
// DriverEntry
// ============================================================
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    SAFETY_BEGIN();
    LOG_INFO("DriverEntry starting");

    LOG_INFO("Resetting trusted partners");
    KeInitializeSpinLock(&g_Service.Lock);
    g_Service.Pid = NULL;

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
    //DECLARE_CONST_UNICODE_STRING(sddl, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)"); // SYSTEM & Admins only

    LOG_INFO("Creating device");
    //WDF_OBJECT_ATTRIBUTES_INIT(&deviceAttributes);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);
    //WdfDeviceInitAssignSDDLString(DeviceInit, &sddl);
    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);
    TRY(WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device));
    g_Device = device;

    LOG_INFO("Creating device context");
    devCtx = DeviceGetContext(device);
    {
        WDF_IO_QUEUE_CONFIG scanRequestQueueConfig;
        WDF_IO_QUEUE_CONFIG_INIT(
            &scanRequestQueueConfig,
            WdfIoQueueDispatchManual
        );
        scanRequestQueueConfig.PowerManaged = WdfFalse;
        TRY(WdfIoQueueCreate(
            device,
            &scanRequestQueueConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &devCtx->ScanRequestQueue
        ));
    }
    InitializeListHead(&devCtx->PendingList);
    {
        WDF_OBJECT_ATTRIBUTES lockAttr;
        WDF_OBJECT_ATTRIBUTES_INIT(&lockAttr);
        lockAttr.ParentObject = device;
        TRY(WdfSpinLockCreate(&lockAttr, &devCtx->PendingListSpinLock));
    }
    InitializeListHead(&devCtx->BlockOperationList);
    {
        WDF_OBJECT_ATTRIBUTES lockAttr;
        WDF_OBJECT_ATTRIBUTES_INIT(&lockAttr);
        lockAttr.ParentObject = device;
        TRY(WdfWaitLockCreate(&lockAttr, &devCtx->BlockOperationListLock));
    }

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
        // Queues and waitlocks have their parent being the device itself,
        // so they get garbage-collected automatically

        if (FAILED_PREVIOUSLY()) {
            PsSetCreateProcessNotifyRoutineEx(RoutineProcessNotify, /*Remove=*/TRUE);
        }
    });
}

// ============================================================
// Process creation callback
// ============================================================
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID RoutineProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
    UNREFERENCED_PARAMETER(Process);

    SAFETY_BEGIN();
    LOG_INFO("RoutingProcessNotify starting");
    LOG_INFO("Process ID: %p", ProcessId);

    BOOL serviceJustExited = FALSE;
    if (CreateInfo == NULL) {
        // Process terminating - if that is a trusted partner,
        // deregister it
        {
            KIRQL oldIrql = 0;
            BOOL matched = FALSE;
            KeAcquireSpinLock(&g_Service.Lock, &oldIrql);
            if (g_Service.Pid == ProcessId) {
                g_Service.Pid = NULL;
                matched = TRUE;
            }
            KeReleaseSpinLock(&g_Service.Lock, oldIrql);
            if (matched) {
                LOG_INFO("Trusted usermode process %p (Service) exited, untrust this PID", ProcessId);
                LOG_INFO("Removing all leftover queued items for sanity.");
                NEVER_FAIL(EnqueueCleanupTaskWorkItem(g_Device, CLEANUP_ALL_SCAN_TASKS, NULL));
                serviceJustExited = TRUE;
                // Claude:
                // Problem: After a process terminates and you call EnqueueCleanupTaskWorkItem, the cleanup happens asynchronously.
                // A new instance of the Service could register before cleanup completes, leading to state corruption.
                // So: We need to check in EvtConductCleanupTask whether a trusted instance of the Service is registered,
                // and only proceed to cleaning up if that's not the case - which we do.
            }
        }

        SUCCEED_FAST();
    }

    {
        // Check if the Service is there. If not, well, do not do anything
        // to prevent malware (simplicity for now), also remove all scan tasks!
        BOOL isServicePresent = FALSE;
        if (serviceJustExited) {
            isServicePresent = FALSE;
        }
        else {
            KIRQL oldIrql = 0;
            KeAcquireSpinLock(&g_Service.Lock, &oldIrql);
            isServicePresent = (g_Service.Pid != NULL);
            KeReleaseSpinLock(&g_Service.Lock, oldIrql);
        }

        if (!isServicePresent) {
            LOG_INFO("Service is not present, so skipped scanning process with PID = %p", ProcessId);
            SUCCEED_FAST();
        }
    }
    // Defer work to PASSIVE_LEVEL
    NEVER_FAIL(EnqueueScanTaskAddWorkItem(g_Device, ProcessId, CreateInfo->ImageFileName));

    LOG_INFO("RoutingProcessNotify finished successfully");
    SAFETY_END_RETURNING_VOID(NO_CLEANUP);
}

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
static VOID EnqueueCleanupTaskWorkItem(WDFDEVICE Device, INT CleanupTask, PVOID Arg)
{
    UNREFERENCED_PARAMETER(Arg); // TODO: Add an "Arg" field to CLEANUP_ITEM_CONTEXT if needed
    SAFETY_BEGIN();

    WDF_OBJECT_ATTRIBUTES attr;
    WDF_WORKITEM_CONFIG cfg;
    WDFWORKITEM workItem;
    PCLEANUP_ITEM_CONTEXT clCtx = NULL;

    WDF_WORKITEM_CONFIG_INIT(&cfg, EvtConductCleanupTask);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, CLEANUP_ITEM_CONTEXT);
    attr.ParentObject = Device;

    TRY(WdfWorkItemCreate(&cfg, &attr, &workItem));

    {
        // Store context data
        clCtx = CleanupItemGetContext(workItem);
        ASSERT_NOT_NULL(clCtx);
        clCtx->CleanupTask = CleanupTask;
    }

    NEVER_FAIL(WdfWorkItemEnqueue(workItem));

    SAFETY_END_RETURNING_VOID(NO_CLEANUP);
}

VOID EvtConductCleanupTask(WDFWORKITEM WorkItem) {
    SAFETY_BEGIN();
    WILL_USE_LOCK(LPending);
    WILL_USE_LOCK(LBlockOperations);

    WDFDEVICE device = NULL;
    PDEVICE_CONTEXT ctx = NULL;
    PCLEANUP_ITEM_CONTEXT clCtx = NULL;

    device = NEVER_FAIL(WdfWorkItemGetParentObject(WorkItem));
    ctx = NEVER_FAIL(DeviceGetContext(device));
    clCtx = CleanupItemGetContext(WorkItem);
    if (!clCtx) {
        LOG_ERROR("No cleanup item context");
        FAIL_FAST();
    }

    if (CLEANUP_ALL_SCAN_TASKS == clCtx->CleanupTask) {
        // Only clean up if the Service is NOT registered
        BOOL isServicePresent = FALSE;
        {
            KIRQL oldIrql = 0;
            KeAcquireSpinLock(&g_Service.Lock, &oldIrql);
            isServicePresent = g_Service.Pid != NULL;
            KeReleaseSpinLock(&g_Service.Lock, oldIrql);
        }

        if (!isServicePresent) {
            SPINLOCK_ACQUIRE(LPending, ctx->PendingListSpinLock);
            while (!IsListEmpty(&ctx->PendingList)) {
                PLIST_ENTRY entry = RemoveHeadList(&ctx->PendingList);
                if (!entry) continue;
                PSCAN_TASK pScanTask = CONTAINING_RECORD(entry, SCAN_TASK, Link);
                if (!pScanTask) continue;
                ExFreePoolWithTag(pScanTask, 'tstA');
            }
            SPINLOCK_RELEASE(LPending, ctx->PendingListSpinLock);

            WAITLOCK_ACQUIRE(LBlockOperations, ctx->BlockOperationListLock, NULL);
            while (!IsListEmpty(&ctx->BlockOperationList)) {
                PLIST_ENTRY entry = RemoveHeadList(&ctx->BlockOperationList);
                if (!entry) continue;
                PBLOCK_OPERATION pBlockOperation = CONTAINING_RECORD(entry, BLOCK_OPERATION, Link);
                if (!pBlockOperation) continue;
                ExFreePoolWithTag(pBlockOperation, 'tstA');
            }
            WAITLOCK_RELEASE(LBlockOperations, ctx->BlockOperationListLock);
        }
    }
    else {
        LOG_ERROR("Unknown cleanup task: %d", clCtx->CleanupTask);
        FAIL_FAST();
    }

    SAFETY_END_RETURNING_VOID({
        SPINLOCK_CLEANUP(LPending, ctx->PendingListSpinLock);
        WAITLOCK_CLEANUP(LBlockOperations, ctx->BlockOperationListLock);
        WdfObjectDelete(WorkItem);
    });
}

// ============================================================
// Queue a pending executable for scan
// ============================================================
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
static VOID EnqueueScanTaskAddWorkItem(WDFDEVICE Device, HANDLE ProcessId, PCUNICODE_STRING ImagePath)
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

        // Try to get file ID
        if (ImagePath && ImagePath->Length > 0 && ImagePath->Buffer) {
            status = GetFileIdFromPath(
                ImagePath,
                &wiCtx->FileId,
                &wiCtx->VolumeSerialNumber
            );

            if (NT_SUCCESS(status)) {
                LOG_INFO("Got file ID for process %p", ProcessId);
            }
            else {
                // TODO: Review this later
                LOG_WARN(
                    "Failed to get file ID for process %p: status = 0x%08X | Ignoring this process (it probably doesn't exist anymore).",
                    ProcessId, status
                );
                SUCCEED_FAST();
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
VOID EvtAddScanTaskToPendingList(WDFWORKITEM WorkItem)
{
    SAFETY_BEGIN();

    LOG_INFO("EvtAddScanTaskToPendingList starting");

    PSCAN_TASK pScanTask = NULL;
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

    LOG_INFO("Allocating SCAN_TASK in non-paged pool");
    #pragma warning(disable: 4996)
    pScanTask = (PSCAN_TASK)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(SCAN_TASK), 'tstA');
    ASSERT_NOT_NULL(pScanTask);

    LOG_INFO("Initializing SCAN_TASK");
    NEVER_FAIL(RtlZeroMemory(pScanTask, sizeof(SCAN_TASK)));
    pScanTask->Dto.Pid = wiCtx->ProcessId;
    NEVER_FAIL(KeQuerySystemTime(&pScanTask->Dto.Timestamp));
    pScanTask->Dto.FileId = wiCtx->FileId;
    pScanTask->Dto.VolumeSerialNumber = wiCtx->VolumeSerialNumber;

    LOG_INFO("Try to deliver the SCAN_TASK");
    EmitterTryingToCompleteScanRequest(pScanTask);
    // At this point, user-mode component can query scan task via IOCTL
    LOG_INFO("EvtAddScanTaskToPendingList finished successfully");

    SAFETY_END_RETURNING_VOID({
        if (FAILED_PREVIOUSLY()) {
            if (pScanTask != NULL) {
                ExFreePoolWithTag(pScanTask, 'tstA');
            }
        }
        WdfObjectDelete(WorkItem);
    });
}

// ============================================================
// IOCTL handler
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
    if (!device) {
        LOG_ERROR("EvtIoDeviceControl: Failed to get device");
        FAIL_FAST();
    }
    PDEVICE_CONTEXT ctx = DeviceGetContext(device);
    if (!ctx) {
        LOG_ERROR("EvtIoDeviceControl: Failed to get device context");
        FAIL_FAST();
    }

    switch (IoControlCode) {
    case IOCTL_REGISTER_SCANNER:
        HandleIOCTLRegisterScanner(device, ctx, Request, OutputBufferLength, InputBufferLength);
        break;

    case IOCTL_GET_PENDING_EXECUTABLE:
        HandleIOCTLGetPendingExecutable(device, ctx, Request, OutputBufferLength, InputBufferLength);
        break;

    case IOCTL_POST_SCANNING_RESULT:
        HandleIOCTLPostScanningResult(device, ctx, Request, OutputBufferLength, InputBufferLength);
        break;

    case IOCTL_GET_NEXT_BLOCK_OPERATION:
        HandleIOCTLGetNextBlockOperation(device, ctx, Request, OutputBufferLength, InputBufferLength);
        break;

    default:
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
        return;
    }

    LOG_INFO("EvtIoDeviceControl finished successfully");
    SAFETY_END_RETURNING_VOID(NO_CLEANUP);
}

/**
 * Initial handshake from the usermode scanner
 * (initial attestation).
 */
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLRegisterScanner(
    WDFDEVICE device,
    PDEVICE_CONTEXT ctx,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength
) {
    // Push a fake scan task to PendingList, all zeroed out
    // (except the nonce) for handshaking.

    UNREFERENCED_PARAMETER(device);
    UNREFERENCED_PARAMETER(ctx);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    SAFETY_BEGIN();

    HANDLE callerPid = GET_CALLER_ID(Request);

    KIRQL oldIrql = 0;
    KeAcquireSpinLock(&g_Service.Lock, &oldIrql);
    g_Service.Pid = callerPid;
    KeReleaseSpinLock(&g_Service.Lock, oldIrql);

    SAFETY_END_RETURNING_VOID({
        WdfRequestComplete(Request, status);
    });
}

/**
 * Pops off and forwards (to usermode) the next
 * scan task in PendingList.
 * 
 * Also moves this task to ScanningList.
 * 
 * Operates in async mode - when there is no
 * scan task, suspend the request till there is one.
 */
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLGetPendingExecutable(
    WDFDEVICE device,
    PDEVICE_CONTEXT ctx,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength
) {
    UNREFERENCED_PARAMETER(device);
    UNREFERENCED_PARAMETER(ctx);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    ConsumerTryingToCompleteScanRequest(Request);
}

/**
 * Tries to complete a scan request.
 * 
 * The scan task pointer MUST be
 * dynamically allocated with ExAllocatePoolWithTag,
 * with pool type NonPagedPoolNx
 * and the tag being 'tstA'.
 * 
 * This is called both by the event
 * emitter(s) (e.g. process creation
 * callback) and the consumer(s)
 * (e.g. HandleIOCTLGetPendingExecutable)
 * so that either side does not
 * lock each other out in a race
 * like this (cre ChatGPT):
 * 
 * CPU 0 (process callback)        CPU 1 (EvtIoDeviceControl)
 * ---------------------------------------------------------
 * Check: no request waiting
 *                                Check: no event queued
 * Queue event
 *                                Queue request
 * 
 * Result: both are queued, nobody completes anything
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID CompleteScanRequest(WDFREQUEST Request, PSCAN_TASK pScanTask) {
    WDFQUEUE queue = WdfRequestGetIoQueue(Request);
    WDFDEVICE device = WdfIoQueueGetDevice(queue);
    PDEVICE_CONTEXT ctx = DeviceGetContext(device);
    if (!ctx) {
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_STATE);
        return;
    }

    SAFETY_BEGIN();
    ASSERT_NOT_NULL(pScanTask);

    PVOID outBuf = NULL;
    size_t outLen = 0;

    TRY(WdfRequestRetrieveOutputBuffer(Request, sizeof(SCAN_TASK_DTO), &outBuf, &outLen));
    if (outBuf == NULL) {
        FAIL_FAST_WITH_STATUS(STATUS_INVALID_ADDRESS);
    }
    if (outLen < sizeof(SCAN_TASK_DTO)) {
        FAIL_FAST_WITH_STATUS(STATUS_BUFFER_TOO_SMALL);
    }
    RtlZeroMemory(outBuf, sizeof(SCAN_TASK_DTO));

    // Copy DTO to user buffer
    RtlCopyMemory(outBuf, &pScanTask->Dto, sizeof(SCAN_TASK_DTO));
    ExFreePoolWithTag(pScanTask, 'tstA');
    WdfRequestSetInformation(Request, sizeof(SCAN_TASK_DTO));
    SUCCEED_FAST();

    SAFETY_END_RETURNING_VOID({
        WdfRequestComplete(Request, status);
    });
}

/**
 * The version of CompleteScanRequest
 * for the event emitters, which only have
 * the event (scan task) at hand.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID EmitterTryingToCompleteScanRequest(PSCAN_TASK pScanTask) {
    WDFDEVICE Device = g_Device;
    PDEVICE_CONTEXT ctx = DeviceGetContext(Device);
    if (!ctx) {
        LOG_ERROR("Context is NULL ???");
        return;
    }

    WDFREQUEST Request = NULL;
    // Try to get a waiting request
    if (
        !NT_SUCCESS(
            WdfIoQueueRetrieveNextRequest(
                ctx->ScanRequestQueue,
                &Request
            )
        )
    ) {
        // No request? Queue it!
        WdfSpinLockAcquire(ctx->PendingListSpinLock);
        InsertTailList(&ctx->PendingList, &pScanTask->Link);
        WdfSpinLockRelease(ctx->PendingListSpinLock);
        return;
    }

    CompleteScanRequest(Request, pScanTask);
}

/**
 * The version of CompleteScanRequest
 * for the event consumers, which only have
 * the WDFREQUEST Request object at hand.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID ConsumerTryingToCompleteScanRequest(WDFREQUEST Request) {
    WDFDEVICE Device = g_Device;

    WILL_USE_LOCK(LPending);

    SAFETY_BEGIN();
    PDEVICE_CONTEXT ctx = DeviceGetContext(Device);
    ASSERT_NOT_NULL(ctx);

    // Try to get a pending scan task
    SPINLOCK_ACQUIRE(LPending, ctx->PendingListSpinLock);
    if (IsListEmpty(&ctx->PendingList)) {
        SPINLOCK_RELEASE(LPending, ctx->PendingListSpinLock);
        WdfRequestForwardToIoQueue(Request, ctx->ScanRequestQueue);
        SUCCEED_FAST();
    }
    else {
        PLIST_ENTRY entry = RemoveHeadList(&ctx->PendingList);
        SPINLOCK_RELEASE(LPending, ctx->PendingListSpinLock);
        ASSERT_NOT_NULL(entry);
        PSCAN_TASK pScanTask = CONTAINING_RECORD(entry, SCAN_TASK, Link);
        ASSERT_NOT_NULL(pScanTask);
        CompleteScanRequest(Request, pScanTask);
    }

    SAFETY_END_RETURNING_VOID({
        if (ctx) {
            SPINLOCK_CLEANUP(LPending, ctx->PendingListSpinLock);
        }
    });
}



/**
 * Gets the scanning result from usermode
 * component, check attestation and take
 * action.
 */
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLPostScanningResult(
    WDFDEVICE device,
    PDEVICE_CONTEXT ctx,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength
) {
    UNREFERENCED_PARAMETER(device);
    //UNREFERENCED_PARAMETER(ctx);
    //UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    SAFETY_BEGIN();
    WILL_USE_LOCK(LPending);
    WILL_USE_LOCK(LBlockOperations);

    PSCAN_VERDICT_DTO verdict = NULL;
    size_t inBufLen = 0;
    PBLOCK_OPERATION pBlockOperation = NULL;

    LOG_INFO("HandleIOCTLPostScanningResult starting");

    //
    // Step 1 — Get input buffer from usermode (SCAN_VERDICT_DTO)
    //
    TRY(WdfRequestRetrieveInputBuffer(
        Request,
        sizeof(SCAN_VERDICT_DTO),
        (PVOID*)&verdict,
        &inBufLen
    ));

    if (inBufLen < sizeof(SCAN_VERDICT_DTO)) {
        LOG_ERROR("Invalid input buffer length %zu", inBufLen);
        FAIL_FAST_WITH_STATUS(STATUS_INVALID_BUFFER_SIZE);
    }
    ASSERT_NOT_NULL(verdict);

    //
    // Step 2 — Handle allow/deny decision or recovery
    //
    // TODO: We may need to also check callerPid == g_Service.Pid

    // Execute the decision:
    if (verdict->AllowExecution) {
        LOG_INFO("Process %p allowed to execute.", verdict->Pid);
    }
    else {
        NTSTATUS terminationStatus = TerminateProcessByPid(verdict->Pid, STATUS_ACCESS_DENIED);
        if (NT_SUCCESS(terminationStatus)) {
            LOG_INFO("Process %p denied by the Service and terminated by the Driver.", verdict->Pid);
        }
        else {
            LOG_INFO("Process %p denied by the Service but could NOT be terminated by the Driver. The incident has been reported.", verdict->Pid);
        }

        //
        // Step 3 — Report Block Operation
        //
        {
            #pragma warning(disable: 4996)
            pBlockOperation = (PBLOCK_OPERATION)ExAllocatePoolWithTag(
                NonPagedPoolNx,
                sizeof(BLOCK_OPERATION),
                TAG_BLOCK_OPERATION
            );
            ASSERT_NOT_NULL(pBlockOperation);

            NEVER_FAIL(RtlZeroMemory(pBlockOperation, sizeof(BLOCK_OPERATION)));
            pBlockOperation->Dto.AllowExecution = verdict->AllowExecution;
            NEVER_FAIL(RtlCopyMemory(pBlockOperation->Dto.PredScore, verdict->PredScore, SIZEOF_DOUBLE));
            pBlockOperation->Dto.FileId = verdict->FileId;
            pBlockOperation->Dto.VolumeSerialNumber = verdict->VolumeSerialNumber;
            pBlockOperation->Dto.IsMitigated = NT_SUCCESS(terminationStatus);
            pBlockOperation->Dto.Version = 1;
            NEVER_FAIL(KeQuerySystemTime(&pBlockOperation->Dto.Timestamp));

            WAITLOCK_ACQUIRE(LBlockOperations, ctx->BlockOperationListLock, NULL);
            NEVER_FAIL(InsertTailList(&ctx->BlockOperationList, &pBlockOperation->Link));
            WAITLOCK_RELEASE(LBlockOperations, ctx->BlockOperationListLock);
            pBlockOperation = NULL;
        }
    }

    LOG_INFO("HandleIOCTLPostScanningResult finished");

    SAFETY_END_RETURNING_VOID({
        SPINLOCK_CLEANUP(LPending, ctx->PendingListSpinLock);
        WAITLOCK_CLEANUP(LBlockOperations, ctx->BlockOperationListLock);

        if (FAILED_PREVIOUSLY()) {
            if (pBlockOperation != NULL) {
                ExFreePoolWithTag(pBlockOperation, TAG_BLOCK_OPERATION);
                pBlockOperation = NULL;
            }
        }

        WdfRequestComplete(Request, status);
    });
}

/**
 * For the UI component: read the next decision
 * (allow/deny execution) of the driver, from
 * oldest to latest.
 */
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLGetNextBlockOperation(
    WDFDEVICE device,
    PDEVICE_CONTEXT ctx,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength
) {
    UNREFERENCED_PARAMETER(device);
    UNREFERENCED_PARAMETER(InputBufferLength);

    SAFETY_BEGIN();
    WILL_USE_LOCK(LBlockOperations);

    PBLOCK_OPERATION pBlockOperation = NULL;
    PLIST_ENTRY entry = NULL;
    PVOID outBuf = NULL;
    size_t outLen = 0;

    if (OutputBufferLength < sizeof(BLOCK_OPERATION_DTO)) {
        FAIL_FAST_WITH_STATUS(STATUS_BUFFER_TOO_SMALL);
    }
    TRY(WdfRequestRetrieveOutputBuffer(Request, sizeof(BLOCK_OPERATION_DTO), &outBuf, &outLen));
    if (outBuf == NULL) {
        FAIL_FAST_WITH_STATUS(STATUS_INVALID_ADDRESS);
    }
    if (outLen < sizeof(BLOCK_OPERATION_DTO)) {
        FAIL_FAST_WITH_STATUS(STATUS_BUFFER_TOO_SMALL);
    }
    RtlZeroMemory(outBuf, sizeof(BLOCK_OPERATION_DTO));

    WAITLOCK_ACQUIRE(LBlockOperations, ctx->BlockOperationListLock, NULL);
    if (IsListEmpty(&ctx->BlockOperationList)) {
        // Nothing to return - leave the output buffer zeroed (done above)
        SUCCEED_FAST();
    }
    entry = RemoveHeadList(&ctx->BlockOperationList);
    WAITLOCK_RELEASE(LBlockOperations, ctx->BlockOperationListLock);

    pBlockOperation = CONTAINING_RECORD(entry, BLOCK_OPERATION, Link);
    ASSERT_NOT_NULL(pBlockOperation);

    // Copy DTO to user buffer
    RtlCopyMemory(outBuf, &pBlockOperation->Dto, sizeof(BLOCK_OPERATION_DTO));
    WdfRequestSetInformation(Request, sizeof(BLOCK_OPERATION_DTO));

    ExFreePoolWithTag(pBlockOperation, TAG_BLOCK_OPERATION);
    pBlockOperation = NULL;

    SUCCEED_FAST();

    SAFETY_END_RETURNING_VOID({
        WAITLOCK_CLEANUP(LBlockOperations, ctx->BlockOperationListLock);

        if (pBlockOperation) {
            ExFreePoolWithTag(pBlockOperation, TAG_BLOCK_OPERATION);
            pBlockOperation = NULL;
        }

        WdfRequestComplete(Request, status);
    });
}










_Function_class_(EVT_WDF_DRIVER_UNLOAD)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID DriverUnload(_In_ WDFDRIVER driver)
{
    UNREFERENCED_PARAMETER(driver);
    SAFETY_BEGIN();
    WILL_USE_LOCK(LPending);
    WILL_USE_LOCK(LBlockOperations);

    PsSetCreateProcessNotifyRoutineEx(RoutineProcessNotify, /*Remove=*/TRUE);

    PDEVICE_CONTEXT ctx = NULL;

    if (g_Device) {
        ctx = DeviceGetContext(g_Device);
        if (ctx) {
            SPINLOCK_ACQUIRE(LPending, ctx->PendingListSpinLock);
            while (!IsListEmpty(&ctx->PendingList)) {
                PLIST_ENTRY entry = RemoveHeadList(&ctx->PendingList);
                ASSERT_NOT_NULL(entry);
                PSCAN_TASK pScanTask = CONTAINING_RECORD(entry, SCAN_TASK, Link);
                ASSERT_NOT_NULL(pScanTask);
                ExFreePoolWithTag(pScanTask, 'tstA');
            }
            SPINLOCK_RELEASE(LPending, ctx->PendingListSpinLock);

            WAITLOCK_ACQUIRE(LBlockOperations, ctx->BlockOperationListLock, NULL);
            while (!IsListEmpty(&ctx->BlockOperationList)) {
                PLIST_ENTRY entry = RemoveHeadList(&ctx->BlockOperationList);
                ASSERT_NOT_NULL(entry);
                PBLOCK_OPERATION pBlockOperation = CONTAINING_RECORD(entry, BLOCK_OPERATION, Link);
                ASSERT_NOT_NULL(pBlockOperation);
                ExFreePoolWithTag(pBlockOperation, TAG_BLOCK_OPERATION);
            }
            WAITLOCK_RELEASE(LBlockOperations, ctx->BlockOperationListLock);
        }
    }

    SAFETY_END_RETURNING_VOID({
        SPINLOCK_CLEANUP(LPending, ctx->PendingListSpinLock);
        WAITLOCK_CLEANUP(LBlockOperations, ctx->BlockOperationListLock);
    });

    // No need to deinitialize spin locks.
}








// TODO: Review this
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS GetFileIdFromPath(PCUNICODE_STRING FilePath, PFILE_ID_128 FileId, PULONG VolumeSerial) {
    NTSTATUS status;
    HANDLE fileHandle = NULL;
    OBJECT_ATTRIBUTES objAttr;
    IO_STATUS_BLOCK iosb;
    PVOID buffer = NULL;

    if (!FilePath || !FilePath->Buffer || !VolumeSerial) return STATUS_INVALID_PARAMETER;

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

// TODO: Might need to use BOOLEAN instead of BOOL everywhere
