#pragma once
#include <windows.h>

void InstallOrReplaceService(
    PCWSTR serviceName,
    PCWSTR displayName,
    DWORD dwStartType,
    PWSTR pszDependencies, 
    PWSTR pszAccount, 
    PWSTR pszPassword,
    BOOL startImmediately
);

bool StartServiceAndWait(SC_HANDLE schService, DWORD timeoutMs = 30000);

bool ServiceExists(LPCWSTR serviceName);

bool UninstallServiceAndWait(LPCWSTR serviceName);
