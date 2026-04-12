#include "drivertools.h"
#include <ntifs.h>

HANDLE GetCallerIDFromIRP(PVOID irpVoid) {
    // PIRP irp = WdfRequestWdmGetIrp(Request);
    PIRP irp = (PIRP)irpVoid;
    ULONG pid32 = IoGetRequestorProcessId(irp);
    HANDLE callerPid = ULongToHandle(pid32);
    return callerPid;
}
