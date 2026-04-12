#pragma once

#include <windef.h>

#ifndef NTSTATUS
#include <ntstatus.h>
#endif

NTSTATUS TerminateProcessByPid(HANDLE Pid, NTSTATUS ExitStatus);
