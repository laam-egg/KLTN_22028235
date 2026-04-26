#include <ntifs.h>
#include "proctools.h"
#include <bcrypt.h>

#ifndef PROCESS_TERMINATE
// https://learn.microsoft.com/en-us/windows/win32/ProcThread/process-security-and-access-rights
#define PROCESS_TERMINATE 0x0001
#endif // PROCESS_TERMINATE

#define NOP() (void)0
#define TRY(status) { if (!NT_SUCCESS(status)) goto cleanup; } NOP()
#define FAIL_FAST_WITH_STATUS(x) { status = x; goto cleanup; } NOP()

#define _LOG(type, prefix, ...) { KdPrintEx((DPFLTR_IHVDRIVER_ID, type, prefix __VA_ARGS__)); KdPrintEx((DPFLTR_IHVDRIVER_ID, type, "\n")); } NOP()
#define LOG_INFO(...)   _LOG(DPFLTR_INFO_LEVEL, "@@@ driver1: INFO: ", __VA_ARGS__)
#define LOG_ERROR(...)  _LOG(DPFLTR_ERROR_LEVEL, "@@@ driver1: ERROR: ", __VA_ARGS__)
#define LOG_WARN(...)   _LOG(DPFLTR_WARNING_LEVEL, "@@@ driver1: WARNING: ", __VA_ARGS__)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS TerminateProcessByPid(HANDLE Pid, NTSTATUS ExitStatus)
{
    NTSTATUS status;
    PEPROCESS process = NULL;
    HANDLE procHandle = NULL;
    BOOLEAN processReferenced = FALSE;

    status = PsLookupProcessByProcessId(Pid, &process);
    TRY(status);
    processReferenced = TRUE;

    // Check if the process is already exiting/terminated
    if (PsGetProcessExitStatus(process) != STATUS_PENDING) {
        LOG_WARN("Process (PID: %p) is already terminating; treating as success.", Pid);
        FAIL_FAST_WITH_STATUS(STATUS_PROCESS_IS_TERMINATING);
    }

    // Open a kernel handle to the process object with PROCESS_TERMINATE
    status = ObOpenObjectByPointer(
        process,
        OBJ_KERNEL_HANDLE,
        NULL,
        PROCESS_TERMINATE,
        *PsProcessType,
        KernelMode,
        &procHandle
    );

    // Dereference the EPROCESS (always done after PsLookupProcessByProcessId)
    ObDereferenceObject(process);
    processReferenced = FALSE;

    TRY(status);

    // Terminate the process
    status = ZwTerminateProcess(procHandle, ExitStatus);
    TRY(status);

cleanup:
    if (procHandle != NULL) {
        ZwClose(procHandle);
    }

    // Handle case where we jumped to cleanup before dereferencing
    if (processReferenced && process != NULL) {
        ObDereferenceObject(process);
    }

    if (status == STATUS_PROCESS_IS_TERMINATING) {
        LOG_INFO("STATUS_PROCESS_IS_TERMINATING is being treated as STATUS_SUCCESS.");
        status = STATUS_SUCCESS;
    }

    return status;
}







////////////////////////////////////////////////////////////////////////////////////////////







_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS VerifyExecutableIntegrity(PCUNICODE_STRING Path, PUCHAR ExpectedHash, ULONG ExpectedHashLen)
{
    UNREFERENCED_PARAMETER(ExpectedHashLen);

    NTSTATUS status;
    HANDLE hFile = NULL;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER byteOffset = { 0 };
    UCHAR buffer[PAGE_SIZE];
    //ULONG bytesRead;

    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    UCHAR hash[32]; // SHA256
    ULONG hashLen = sizeof(hash);
    if (ExpectedHashLen != hashLen) {
        FAIL_FAST_WITH_STATUS(STATUS_INVALID_PARAMETER);
    }

    InitializeObjectAttributes(&oa, (PUNICODE_STRING)Path, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwCreateFile(
        &hFile,
        GENERIC_READ,
        &oa,
        &iosb,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
    );
    TRY(status);

    TRY(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0));
    TRY(BCryptCreateHash(hAlg, &hHash, NULL, 0, NULL, 0, 0));

    do {
        status = ZwReadFile(hFile, NULL, NULL, NULL, &iosb, buffer, sizeof(buffer), &byteOffset, NULL);
        if (NT_SUCCESS(status) && iosb.Information > 0) {
            BCryptHashData(hHash, buffer, (ULONG)iosb.Information, 0);
            byteOffset.QuadPart += iosb.Information;
        }
    } while (NT_SUCCESS(status) && iosb.Information > 0);

    TRY(BCryptFinishHash(hHash, hash, hashLen, 0));

    if (RtlCompareMemory(hash, ExpectedHash, hashLen) != hashLen) {
        status = STATUS_ACCESS_DENIED;
    }

cleanup:
    if (hHash) BCryptDestroyHash(hHash);
    if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
    if (hFile) ZwClose(hFile);
    return status;
}
