#pragma once

#include <windows.h>
#include <winioctl.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include "config.h"

HANDLE OpenDriverDevice();
bool GetBlockOperation(HANDLE hDevice, BLOCK_OPERATION_DTO* dto);
void CloseDriverDevice(HANDLE hDevice);
