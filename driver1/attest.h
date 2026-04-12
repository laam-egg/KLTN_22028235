#pragma once

//#include <windef.h>
//
//#ifndef NTSTATUS
//#include <ntstatus.h>
//#endif

#define DRIVER1_KERNEL_MODE
#include "config.h"

NTSTATUS VerifyAttestation(
	PSCAN_TASK_DTO pScanTaskDto,
	PSCAN_VERDICT_DTO pScanVerdictDto
);
