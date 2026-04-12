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
