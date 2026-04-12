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
#include "attest.h"
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

TRUSTED_PARTNER g_Model = { 0 };
TRUSTED_PARTNER g_UI = { 0 }; // currently the UI component is not yet required to attest, so ignore it

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
#define LOG_WARN(...)   _LOG(DPFLTR_WARNING_LEVEL, "@@@ driver1: WARNING: ", __VA_ARGS__)

inline HANDLE GET_CALLER_ID(WDFREQUEST Request) {
    PIRP irp = WdfRequestWdmGetIrp(Request);
    return GetCallerIDFromIRP(irp);
}

#define MAX_RETRIES 5
#define TAG_DECISION 'dec '

#define CLEANUP_ALL_SCAN_TASKS 1

///////////////////////////////////
/////// Other Declarations ////////
///////////////////////////////////

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD DriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;
EVT_WDF_WORKITEM EvtAddScanTaskToPendingList;
EVT_WDF_WORKITEM EvtConductCleanupTask;

VOID RoutineProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);

VOID EnqueueScanTaskAddWorkItem(WDFDEVICE Device, HANDLE ProcessId, PCUNICODE_STRING ImagePath);

VOID EnqueueCleanupTaskWorkItem(WDFDEVICE Device, INT CleanupTask, PVOID Arg);

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

    /**
     * List of past decisions made by this driver i.e. allow/deny
     * execution, of which files, when... Mean to be read via
     * IOCTL_GET_NEXT_DECISION.
     */
    LIST_ENTRY DecisionList;

    /**
     * Lock for DecisionList. Use in PASSIVE_LEVEL only.
     */
    WDFWAITLOCK DecisionListLock;
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

////// SCAN TASK (AN ELEMENT IN PendingList or ScanningList OF DEVICE CONTEXT) //////////

typedef struct _SCAN_TASK {
    SCAN_TASK_DTO Dto;
    LIST_ENTRY Link;
    HANDLE CallerPid;
    INT8 Retried;
} SCAN_TASK, * PSCAN_TASK;

////// DECISION (AN ELEMENT IN DecisionList OF DEVICE CONTEXT) //////////

typedef struct _DECISION {
    DECISION_DTO Dto;
    LIST_ENTRY Link;
} DECISION, *PDECISION;

//////////// IOCTL HANDLERS (CONTEXT-DEPENDENT) /////////////

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLRegisterScanner(
    WDFDEVICE device,
    PDEVICE_CONTEXT ctx,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength
);

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

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLGetNextDecision(
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
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS EvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    PAGED_CODE() // TODO: Add this for other PASSIVE_LEVEL routines too?
    UNREFERENCED_PARAMETER(Driver);

    LOG_INFO("EvtDeviceAdd starting");

    SAFETY_BEGIN();

    LOG_INFO("Resetting trusted partners");
    KeInitializeSpinLock(&g_Model.Lock);
    g_Model.Pid = NULL;
    KeInitializeSpinLock(&g_UI.Lock);
    g_UI.Pid = NULL;

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
    InitializeListHead(&devCtx->DecisionList);
    TRY(WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &devCtx->DecisionListLock));

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
VOID RoutineProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
    UNREFERENCED_PARAMETER(Process);

    SAFETY_BEGIN();
    LOG_INFO("RoutingProcessNotify starting");
    LOG_INFO("Process ID: %p", ProcessId);

    if (CreateInfo == NULL) {
        // Process terminating - if that is a trusted partner,
        // deregister it
        {
            KIRQL oldIrql = 0;
            BOOL matched = FALSE;
            KeAcquireSpinLock(&g_Model.Lock, &oldIrql);
            if (g_Model.Pid == ProcessId) {
                g_Model.Pid = NULL;
                matched = TRUE;
            }
            KeReleaseSpinLock(&g_Model.Lock, oldIrql);
            if (matched) {
                LOG_INFO("Trusted usermode process %p (AI model) exited, untrust this PID", ProcessId);
                LOG_INFO("Removing all leftover scan tasks for sanity.");
                NEVER_FAIL(EnqueueCleanupTaskWorkItem(g_Device, CLEANUP_ALL_SCAN_TASKS, NULL));
                // Claude:
                // Problem: After a process terminates and you call EnqueueCleanupTaskWorkItem, the cleanup happens asynchronously.A new instance of the AI model could register before cleanup completes, leading to state corruption.
                // So: We need to check in EvtConductCleanupTask whether a trusted instance of the AI model is registered, and only proceed to cleaning up if that's not the case - which we do.
            }
        }

        {
            KIRQL oldIrql = 0;
            BOOL matched = FALSE;
            KeAcquireSpinLock(&g_UI.Lock, &oldIrql);
            if (g_UI.Pid == ProcessId) {
                g_UI.Pid = NULL;
                matched = TRUE;
            }
            KeReleaseSpinLock(&g_UI.Lock, oldIrql);
            if (matched) {
                LOG_INFO("Trusted usermode process %p (UI) exited, driver enters rest", ProcessId);
            }
        }
        
        SUCCEED_FAST();
    }

    {
        // Check if the AI model is there. If not, well, do not do anything
        // to prevent malware (simplicity for now), also remove all scan tasks!
        BOOL isModelPresent = FALSE;
        KIRQL oldIrql = 0;
        KeAcquireSpinLock(&g_Model.Lock, &oldIrql);
        isModelPresent = (g_Model.Pid != NULL);
        KeReleaseSpinLock(&g_Model.Lock, oldIrql);
        if (!isModelPresent) {
            LOG_INFO("AI model is not present, so skipped scanning process with PID = %p", ProcessId);
            SUCCEED_FAST();
        }
    }
    // Defer work to PASSIVE_LEVEL
    NEVER_FAIL(EnqueueScanTaskAddWorkItem(g_Device, ProcessId, CreateInfo->ImageFileName));

    LOG_INFO("RoutingProcessNotify finished successfully");
    SAFETY_END_RETURNING_VOID(NO_CLEANUP);
}

VOID EnqueueCleanupTaskWorkItem(WDFDEVICE Device, INT CleanupTask, PVOID Arg)
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

_IRQL_requires_max_(PASSIVE_LEVEL) // The rule: IRQL annotations describe the required entry IRQL ; NOT the max IRQL that this function itself could raise upto
VOID EvtConductCleanupTask(WDFWORKITEM WorkItem) {
    SAFETY_BEGIN();
    WILL_USE_LOCK(LPending);
    WILL_USE_LOCK(LScanning);

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
        // Only clean up if the AI model is NOT registered
        BOOL isAIModelPresent = FALSE;
        {
            KIRQL oldIrql = 0;
            KeAcquireSpinLock(&g_Model.Lock, &oldIrql);
            isAIModelPresent = g_Model.Pid != NULL;
            KeReleaseSpinLock(&g_Model.Lock, oldIrql);
        }

        if (!isAIModelPresent) {
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
    else {
        LOG_ERROR("Unknown cleanup task: %d", clCtx->CleanupTask);
        FAIL_FAST();
    }

    SAFETY_END_RETURNING_VOID({
        WAITLOCK_CLEANUP(LPending, ctx->PendingListLock);
        WAITLOCK_CLEANUP(LScanning, ctx->ScanningListLock);
        WdfObjectDelete(WorkItem);
    });
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
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID EvtAddScanTaskToPendingList(WDFWORKITEM WorkItem)
{
    SAFETY_BEGIN();
    WILL_USE_LOCK(LPending);

    LOG_INFO("EvtAddScanTaskToPendingList starting");

    PSCAN_TASK pScanTask = NULL;
    BYTE nonce[32];
    WDFDEVICE device = NULL;
    PDEVICE_CONTEXT ctx = NULL;
    PWORK_ITEM_CONTEXT wiCtx = NULL;

    // Only add if the AI model is NOT registered
    {
        BOOL isAIModelPresent = FALSE;

        KIRQL oldIrql = 0;
        KeAcquireSpinLock(&g_Model.Lock, &oldIrql);
        isAIModelPresent = g_Model.Pid != NULL;
        KeReleaseSpinLock(&g_Model.Lock, oldIrql);

        if (!isAIModelPresent) {
            SUCCEED_FAST();
        }
    }

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

    LOG_INFO("Initializing attestation entry");
    NEVER_FAIL(RtlZeroMemory(pScanTask, sizeof(SCAN_TASK)));
    NEVER_FAIL(RtlCopyMemory(pScanTask->Dto.Nonce, nonce, sizeof(nonce)));
    pScanTask->Dto.Pid = wiCtx->ProcessId;
    NEVER_FAIL(KeQuerySystemTime(&pScanTask->Dto.Timestamp));
    pScanTask->Dto.FileId = wiCtx->FileId;
    pScanTask->Dto.VolumeSerialNumber = wiCtx->VolumeSerialNumber;

    LOG_INFO("Acquiring lock to send attest entry into queue");
    WAITLOCK_ACQUIRE(LPending, ctx->PendingListLock, NULL);
    NEVER_FAIL(InsertTailList(&ctx->PendingList, &pScanTask->Link));
    WAITLOCK_RELEASE(LPending, ctx->PendingListLock);

    // At this point, user-mode component can query scan task via IOCTL
    LOG_INFO("Nonce generated and pending entry inserted for PID %p", pScanTask->Dto.Pid);
    LOG_INFO("EvtAddScanTaskToPendingList finished successfully");

    SAFETY_END_RETURNING_VOID({
        if (FAILED_PREVIOUSLY()) {
            if (pScanTask != NULL) {
                ExFreePoolWithTag(pScanTask, 'tstA');
            }
        }
        WAITLOCK_CLEANUP(LPending, ctx->PendingListLock);
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

    case IOCTL_GET_NEXT_DECISION:
        HandleIOCTLGetNextDecision(device, ctx, Request, OutputBufferLength, InputBufferLength);
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
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLRegisterScanner(
    WDFDEVICE device,
    PDEVICE_CONTEXT ctx,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength
) {
    // Push a fake scan task to PendingList, all zeroed out,
    // for handshaking.

    UNREFERENCED_PARAMETER(device);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    SAFETY_BEGIN();
    WILL_USE_LOCK(LPending);

    HANDLE callerPid = GET_CALLER_ID(Request);

    PSCAN_TASK pScanTask = NULL;

    #pragma warning(disable: 4996)
    pScanTask = (PSCAN_TASK)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(SCAN_TASK), 'tstA');
    ASSERT_NOT_NULL(pScanTask);

    NEVER_FAIL(RtlZeroMemory(pScanTask, sizeof(SCAN_TASK)));
    pScanTask->CallerPid = callerPid;

    LOG_INFO("Acquiring lock to send attest entry into queue");
    WAITLOCK_ACQUIRE(LPending, ctx->PendingListLock, NULL);
    NEVER_FAIL(InsertTailList(&ctx->PendingList, &pScanTask->Link));
    WAITLOCK_RELEASE(LPending, ctx->PendingListLock);

    // At this point, user-mode component can query scan task via IOCTL

    SAFETY_END_RETURNING_VOID({
        if (FAILED_PREVIOUSLY()) {
            if (pScanTask != NULL) {
                ExFreePoolWithTag(pScanTask, 'tstA');
            }
        }
        WAITLOCK_CLEANUP(LPending, ctx->PendingListLock);
        WdfRequestComplete(Request, status);
    });
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
    UNREFERENCED_PARAMETER(InputBufferLength);

    SAFETY_BEGIN();
    WILL_USE_LOCK(LPending);
    WILL_USE_LOCK(LScanning);

    PSCAN_TASK pScanTask = NULL;
    PLIST_ENTRY entry = NULL;
    PVOID outBuf = NULL;
    size_t outLen = 0;

    if (OutputBufferLength < sizeof(SCAN_TASK_DTO)) {
        FAIL_FAST_WITH_STATUS(STATUS_BUFFER_TOO_SMALL);
    }
    TRY(WdfRequestRetrieveOutputBuffer(Request, sizeof(SCAN_TASK_DTO), &outBuf, &outLen));
    if (outBuf == NULL) {
        FAIL_FAST_WITH_STATUS(STATUS_INVALID_ADDRESS);
    }
    if (outLen < sizeof(SCAN_TASK_DTO)) {
        FAIL_FAST_WITH_STATUS(STATUS_BUFFER_TOO_SMALL);
    }
    RtlZeroMemory(outBuf, sizeof(SCAN_TASK_DTO));

    // Acquire PendingList lock and pop head
    WAITLOCK_ACQUIRE(LPending, ctx->PendingListLock, NULL);
    if (IsListEmpty(&ctx->PendingList)) {
        // Nothing to return - leave the output buffer zeroed (done above)
        SUCCEED_FAST();
    }

    entry = RemoveHeadList(&ctx->PendingList);
    WAITLOCK_RELEASE(LPending, ctx->PendingListLock);

    pScanTask = CONTAINING_RECORD(entry, SCAN_TASK, Link);
    ASSERT_NOT_NULL(pScanTask);

    // Move the task to ScanningList under ScanningListLock
    WAITLOCK_ACQUIRE(LScanning, ctx->ScanningListLock, NULL);
    InsertTailList(&ctx->ScanningList, &pScanTask->Link);
    WAITLOCK_RELEASE(LScanning, ctx->ScanningListLock);

    // Copy DTO to user buffer
    RtlCopyMemory(outBuf, &pScanTask->Dto, sizeof(SCAN_TASK_DTO));
    WdfRequestSetInformation(Request, sizeof(SCAN_TASK_DTO));
    SUCCEED_FAST();

    SAFETY_END_RETURNING_VOID({
        WAITLOCK_CLEANUP(LPending, ctx->PendingListLock);
        WAITLOCK_CLEANUP(LScanning, ctx->ScanningListLock);
        if (FAILED_PREVIOUSLY()) {
            if (pScanTask) {
                // move back to PendingList
                WAITLOCK_ACQUIRE(LPending, ctx->PendingListLock, NULL);
                InsertHeadList(&ctx->PendingList, &pScanTask->Link);
                WAITLOCK_RELEASE(LPending, ctx->PendingListLock);
            }
        }
        WdfRequestComplete(Request, status);
        WAITLOCK_CLEANUP(LPending, ctx->PendingListLock);
        WAITLOCK_CLEANUP(LScanning, ctx->ScanningListLock);
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
    UNREFERENCED_PARAMETER(device);
    //UNREFERENCED_PARAMETER(ctx);
    //UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    SAFETY_BEGIN();
    WILL_USE_LOCK(LScanning);
    WILL_USE_LOCK(LPending);
    WILL_USE_LOCK(LDecision);

    HANDLE callerPid = GET_CALLER_ID(Request);

    PSCAN_VERDICT_DTO verdict = NULL;
    PSCAN_TASK pScanTask = NULL;
    SCAN_TASK_DTO taskCopy = { 0 };
    size_t inBufLen = 0;
    NTSTATUS verifyStatus = STATUS_UNSUCCESSFUL;
    PDECISION pDecision = NULL;

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

    //
    // Step 2 — Locate matching SCAN_TASK in ScanningList (by PID)
    //
    WAITLOCK_ACQUIRE(LScanning, ctx->ScanningListLock, NULL);

    BOOLEAN found = FALSE;
    for (PLIST_ENTRY entry = ctx->ScanningList.Flink;
        entry != &ctx->ScanningList;
        entry = entry->Flink)
    {
        PSCAN_TASK current = CONTAINING_RECORD(entry, SCAN_TASK, Link);
        if (current->Dto.Pid == verdict->Pid) {
            if (verdict->Pid == 0 /*for handshake*/ && current->CallerPid != callerPid /*but inconsistent callerPid*/) {
                /*then this scan task does not match/qualify*/
                continue;
            }
            else {
                found = TRUE;
                pScanTask = current;
                break;
            }
        }
    }

    if (!found) {
        LOG_ERROR("No matching SCAN_TASK for PID %p", verdict->Pid);
        FAIL_FAST_WITH_STATUS(STATUS_NOT_FOUND);
    }

    // Make a safe local copy before verification
    RtlCopyMemory(&taskCopy, &pScanTask->Dto, sizeof(SCAN_TASK_DTO));
    WAITLOCK_RELEASE(LScanning, ctx->ScanningListLock);

    //
    // Step 3 — Verify attestation
    //
    verifyStatus = VerifyAttestation(&taskCopy, verdict);
    if (!NT_SUCCESS(verifyStatus)) {
        LOG_ERROR("Attestation verification failed for PID %p", verdict->Pid);
        #ifndef TAKE_THE_RISK
        TRY(TerminateProcessByPid(verdict->Pid, STATUS_INVALID_SIGNATURE));
        #endif // TAKE_THE_RISK
        SUCCEED_FAST();
    }

    //
    // Step 4 — Handle allow/deny decision or recovery
    //
    if (NT_SUCCESS(verifyStatus)) {
        if (!verdict->Pid) {
            // Fake decision for handshake - now that the caller
            // attested correctly => it's the legitimate AI model.
            // Register it as a trusted partner.
            KIRQL oldIrql = 0;
            KeAcquireSpinLock(&g_Model.Lock, &oldIrql);
            g_Model.Pid = callerPid;
            KeReleaseSpinLock(&g_Model.Lock, oldIrql);
        }
        else {
            // Normal decision: allow or block execution.
            // TODO: We may need to also check callerPid == g_Model.Pid
            // but first...

            //
            // Step 5 - Persist the decision
            //
            {
                #pragma warning(disable: 4996)
                pDecision = (PDECISION)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(DECISION), TAG_DECISION);
                ASSERT_NOT_NULL(pDecision);
                NEVER_FAIL(RtlZeroMemory(pDecision, sizeof(DECISION)));
                pDecision->Dto.AllowExecution = verdict->AllowExecution;
                pDecision->Dto.FileId = pScanTask->Dto.FileId;
                pDecision->Dto.VolumeSerialNumber = pScanTask->Dto.VolumeSerialNumber;
                pDecision->Dto.IsValid = TRUE;
                pDecision->Dto.Version = 1;
                NEVER_FAIL(KeQuerySystemTime(&pDecision->Dto.Timestamp));
            }
            WAITLOCK_ACQUIRE(LDecision, ctx->DecisionListLock, NULL);
            NEVER_FAIL(InsertTailList(&ctx->DecisionList, &pDecision->Link));
            WAITLOCK_RELEASE(LDecision, ctx->DecisionListLock);

            // Then, execute the decision.

            if (verdict->AllowExecution) {
                LOG_INFO("Process %p allowed to execute.", verdict->Pid);
            }
            else {
                LOG_INFO("Process %p denied by AI model. Terminating...", verdict->Pid);
                TRY(TerminateProcessByPid(verdict->Pid, STATUS_ACCESS_DENIED));
            }
            WAITLOCK_ACQUIRE(LScanning, ctx->ScanningListLock, NULL);
            RemoveEntryList(&pScanTask->Link);
            WAITLOCK_RELEASE(LScanning, ctx->ScanningListLock);
            ExFreePoolWithTag(pScanTask, 'tstA');
            pScanTask = NULL;

        }
    }

    LOG_INFO("HandleIOCTLPostScanningResult finished");
    SAFETY_END_RETURNING_VOID({
        WAITLOCK_CLEANUP(LScanning, ctx->ScanningListLock);
        WAITLOCK_CLEANUP(LPending, ctx->PendingListLock);
        WAITLOCK_CLEANUP(LDecision, ctx->DecisionListLock);

        if (FAILED_PREVIOUSLY()) {
            if (pDecision != NULL) {
                ExFreePoolWithTag(pDecision, TAG_DECISION);
                pDecision = NULL;
            }
        }

        if (!verifyStatus && pScanTask != NULL) {
            if (!FAILED_PREVIOUSLY()) {
                status = STATUS_UNSUCCESSFUL;
            }
            // Verification failed or other error — requeue for another scan
            if (pScanTask->Retried >= MAX_RETRIES) {
                LOG_ERROR("Failed to scan process %p too many times, ignoring (i.e. let it run) !", pScanTask->Dto.Pid);
                status = STATUS_INTERNAL_ERROR;
            }
            else {
                LOG_WARN("Verification or communication failed, requeuing PID to 'Pending for Scan': %p", verdict->Pid);

                WAITLOCK_ACQUIRE(LScanning, ctx->ScanningListLock, NULL);
                RemoveEntryList(&pScanTask->Link);
                WAITLOCK_RELEASE(LScanning, ctx->ScanningListLock);

                WAITLOCK_ACQUIRE(LPending, ctx->PendingListLock, NULL);
                pScanTask->Retried += 1;
                InsertTailList(&ctx->PendingList, &pScanTask->Link);
                WAITLOCK_RELEASE(LPending, ctx->PendingListLock);
            }
        }

        WdfRequestComplete(Request, status);
        WAITLOCK_CLEANUP(LScanning, ctx->ScanningListLock);
        WAITLOCK_CLEANUP(LPending, ctx->PendingListLock);
        WAITLOCK_CLEANUP(LDecision, ctx->DecisionListLock);
    });
}

/**
 * For the UI component: read the next decision
 * (allow/deny execution) of the driver, from
 * oldest to latest.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID HandleIOCTLGetNextDecision(
    WDFDEVICE device,
    PDEVICE_CONTEXT ctx,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength
) {
    UNREFERENCED_PARAMETER(device);
    UNREFERENCED_PARAMETER(InputBufferLength);

    SAFETY_BEGIN();
    WILL_USE_LOCK(LDecision);

    PDECISION pDecision = NULL;
    PLIST_ENTRY entry = NULL;
    PVOID outBuf = NULL;
    size_t outLen = 0;

    if (OutputBufferLength < sizeof(DECISION_DTO)) {
        FAIL_FAST_WITH_STATUS(STATUS_BUFFER_TOO_SMALL);
    }
    TRY(WdfRequestRetrieveOutputBuffer(Request, sizeof(DECISION_DTO), &outBuf, &outLen));
    if (outBuf == NULL) {
        FAIL_FAST_WITH_STATUS(STATUS_INVALID_ADDRESS);
    }
    if (outLen < sizeof(DECISION_DTO)) {
        FAIL_FAST_WITH_STATUS(STATUS_BUFFER_TOO_SMALL);
    }
    RtlZeroMemory(outBuf, sizeof(DECISION_DTO));

    // Acquire DecisionList lock and pop head
    WAITLOCK_ACQUIRE(LDecision, ctx->DecisionListLock, NULL);
    if (IsListEmpty(&ctx->DecisionList)) {
        // Nothing to return - leave the output buffer zeroed (done above)
        SUCCEED_FAST();
    }
    entry = RemoveHeadList(&ctx->DecisionList);
    WAITLOCK_RELEASE(LDecision, ctx->DecisionListLock);

    pDecision = CONTAINING_RECORD(entry, DECISION, Link);
    ASSERT_NOT_NULL(pDecision);

    // Copy DTO to user buffer
    RtlCopyMemory(outBuf, &pDecision->Dto, sizeof(DECISION_DTO));
    WdfRequestSetInformation(Request, sizeof(DECISION_DTO));

    ExFreePoolWithTag(pDecision, TAG_DECISION);
    pDecision = NULL;

    SUCCEED_FAST();

    SAFETY_END_RETURNING_VOID({
        WAITLOCK_CLEANUP(LDecision, ctx->DecisionListLock);

        if (pDecision) {
            // TODO: Now we accept that the decision might be lost here.
            // No requeue.
            ExFreePoolWithTag(pDecision, TAG_DECISION);
            pDecision = NULL;
        }

        WdfRequestComplete(Request, status);
        WAITLOCK_CLEANUP(LDecision, ctx->DecisionListLock);
    });
}










_IRQL_requires_max_(PASSIVE_LEVEL)
VOID DriverUnload(WDFDRIVER driver)
{
    UNREFERENCED_PARAMETER(driver);
    SAFETY_BEGIN();
    WILL_USE_LOCK(LPending);
    WILL_USE_LOCK(LScanning);
    WILL_USE_LOCK(LDecision);

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

            WAITLOCK_ACQUIRE(LDecision, ctx->DecisionListLock, NULL);
            while (!IsListEmpty(&ctx->DecisionList)) {
                PLIST_ENTRY entry = RemoveHeadList(&ctx->DecisionList);
                ASSERT_NOT_NULL(entry);
                PDECISION pDecision = CONTAINING_RECORD(entry, DECISION, Link);
                ASSERT_NOT_NULL(pDecision);
                ExFreePoolWithTag(pDecision, TAG_DECISION);
            }
            WAITLOCK_RELEASE(LDecision, ctx->DecisionListLock);
        }
    }
    PsSetCreateProcessNotifyRoutineEx(RoutineProcessNotify, TRUE);

    SAFETY_END_RETURNING_VOID({
        WAITLOCK_CLEANUP(LPending, ctx->PendingListLock);
        WAITLOCK_CLEANUP(LScanning, ctx->ScanningListLock);
        WAITLOCK_CLEANUP(LDecision, ctx->DecisionListLock);
    });

    // No need to deinitialize spin locks.
}








// TODO: Review this
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
