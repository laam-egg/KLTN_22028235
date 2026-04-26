#pragma once

#include <windows.h>
#include <winioctl.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include "config.h"

typedef struct _PENDING_SCAN_TASK {
    OVERLAPPED ov;
    SCAN_TASK_DTO dto;
} PENDING_SCAN_TASK;

HANDLE OpenDriverDevice();
bool RegisterWithDriver(HANDLE hDevice);

bool GetPendingTaskAsync(HANDLE hDevice, PENDING_SCAN_TASK* pPendingTask);
bool CompletePendingTask(
    HANDLE hDevice,
    PENDING_SCAN_TASK* pPendingTask,
    SCAN_TASK_DTO* pOutTask
);
void CancelPendingTask(HANDLE hDevice, PENDING_SCAN_TASK* pPendingTask);

bool SendVerdict(HANDLE hDevice, SCAN_VERDICT_DTO* pVerdict);
void CloseDriverDevice(HANDLE hDevice);
