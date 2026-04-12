#pragma once

#include <windef.h>

#ifndef NTSTATUS
#include <ntstatus.h>
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS TerminateProcessByPid(HANDLE Pid, NTSTATUS ExitStatus);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS VerifyExecutableIntegrity(PCUNICODE_STRING Path, PUCHAR ExpectedHash, ULONG ExpectedHashLen);
