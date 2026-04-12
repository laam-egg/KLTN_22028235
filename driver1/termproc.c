#include <ntifs.h>
#include "termproc.h"

#ifndef PROCESS_TERMINATE
// https://learn.microsoft.com/en-us/windows/win32/ProcThread/process-security-and-access-rights
#define PROCESS_TERMINATE 0x0001
#endif // PROCESS_TERMINATE

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS TerminateProcessByPid(HANDLE Pid, NTSTATUS ExitStatus)
{
    NTSTATUS status;
    PEPROCESS process = NULL;
    HANDLE procHandle = NULL;

    status = PsLookupProcessByProcessId(Pid, &process);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Open a kernel handle to the process object with PROCESS_TERMINATE
    status = ObOpenObjectByPointer(
        process,
        OBJ_KERNEL_HANDLE,
        NULL,
        PROCESS_TERMINATE,
        *PsProcessType, // compiler may accept PsProcessType
        KernelMode,
        &procHandle
    );

    // dereference the EPROCESS
    ObDereferenceObject(process);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Terminate. If ZwTerminateProcess expects a handle to process object, pass handle
    status = ZwTerminateProcess(procHandle, ExitStatus);
    ZwClose(procHandle);
    return status;
}
