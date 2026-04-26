#pragma once

#ifndef HANDLE
typedef void* HANDLE;
#endif

#ifndef PVOID
typedef void* PVOID;
#endif

HANDLE GetCallerIDFromIRP(PVOID irp);
