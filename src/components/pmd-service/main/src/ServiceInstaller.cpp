#pragma region "Includes"
#include <stdio.h>
#include <windows.h>
#include "ServiceInstaller.h"
#include "Logging.h"
#pragma endregion

void InstallOrReplaceService(
    PCWSTR lpszServiceName,
    PCWSTR lpszDisplayName,
    DWORD dwStartType,
    PWSTR pszDependencies, 
    PWSTR pszAccount, 
    PWSTR pszPassword,
    BOOL startImmediately
)
{
    DebugLog(L"Installing service: ", lpszServiceName);

    // 1. If exists → remove
    if (false == UninstallServiceAndWait(lpszServiceName)) {
        DebugLog(L"Failed to uninstall service, but proceed with installation anyway.");
    }

    // 2. Get binary path
    wchar_t path[MAX_PATH];
    GetModuleFileName(nullptr, path, MAX_PATH);

    // 3. Create service
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scm) {
        DebugLog(L"OpenSCManager failed: ", GetLastError());
        return;
    }

    SC_HANDLE svc = CreateService(
        scm,                            // SCManager database
        lpszServiceName,                    // Name of service
        lpszDisplayName,                    // Name to display
        SERVICE_ALL_ACCESS,             // Desired access
        SERVICE_WIN32_OWN_PROCESS,      // Service type
        dwStartType,                    // Service start type
        SERVICE_ERROR_NORMAL,           // Error control type
        path,                           // Service's binary
        NULL,                           // No load ordering group
        NULL,                           // No tag identifier
        pszDependencies,                // Dependencies
        pszAccount,                     // Service running account
        pszPassword                     // Password of the account
    );

    if (!svc)
    {
        DebugLog(L"CreateService failed: ", GetLastError());
    }
    else
    {
        DebugLog(L"Service ", lpszServiceName, L" installed successfully");
        if (startImmediately) {
            StartServiceAndWait(svc);
        }
        CloseServiceHandle(svc);
    }


    CloseServiceHandle(scm);
}

bool StartServiceAndWait(SC_HANDLE schService, DWORD timeoutMs)
{
    DebugLog(L"Starting service...");

    if (!StartService(schService, 0, nullptr))
    {
        DWORD err = GetLastError();

        // Service might already be running
        if (err != ERROR_SERVICE_ALREADY_RUNNING)
        {
            DebugLog(L"StartService failed: ", err);
            return false;
        }
    }

    SERVICE_STATUS_PROCESS ssp = {};
    DWORD bytesNeeded = 0;
    DWORD startTick = GetTickCount();

    while (true)
    {
        if (!QueryServiceStatusEx(
                schService,
                SC_STATUS_PROCESS_INFO,
                reinterpret_cast<LPBYTE>(&ssp),
                sizeof(ssp),
                &bytesNeeded))
        {
            DebugLog(L"QueryServiceStatusEx failed: ", GetLastError());
            return false;
        }

        if (ssp.dwCurrentState == SERVICE_RUNNING)
        {
            DebugLog(L"Service is running.");
            return true;
        }

        if (ssp.dwCurrentState == SERVICE_STOPPED)
        {
            DebugLog(L"Service failed to start.");
            return false;
        }

        if (GetTickCount() - startTick > timeoutMs)
        {
            DebugLog(L"Service start timed out.");
            return false;
        }

        Sleep(300);
    }
}

bool ServiceExists(LPCWSTR serviceName)
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL)
    {
        return false;
    }

    SC_HANDLE schService = OpenService(schSCManager, serviceName, SERVICE_QUERY_STATUS);
    if (schService == NULL)
    {
        CloseServiceHandle(schSCManager);
        return false;
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return true;
}

bool UninstallServiceAndWait(LPCWSTR serviceName)
{
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scm) {
        DebugLog(L"OpenSCManager failed: ", GetLastError());
        return false;
    }

    SC_HANDLE svc = OpenService(
        scm,
        serviceName,
        SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE
    );

    if (!svc)
    {
        CloseServiceHandle(scm);
        if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
            DebugLog(L"Service does not exist: ", serviceName, " ; nothing to uninstall.");
            return true;
        } else {
            DebugLog(L"OpenService failed: ", GetLastError());
            return false;
        }
    }

    SERVICE_STATUS status = {};

    // Stop if running
    if (ControlService(svc, SERVICE_CONTROL_STOP, &status))
    {
        do
        {
            DebugLog(L"Waiting for service to stop...");
            Sleep(500);
            QueryServiceStatus(svc, &status);
        } while (status.dwCurrentState == SERVICE_STOP_PENDING);

        DebugLog(L"Service stopped.");
    }

    // Delete
    DebugLog(L"Deleting service...");
    DeleteService(svc);

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);

    // Wait until SCM fully removes it
    for (;;)
    {
        DebugLog("Waiting for service to be fully removed by SCM...");
        scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
        svc = OpenService(scm, serviceName, SERVICE_QUERY_STATUS);

        if (!svc && GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
        {
            CloseServiceHandle(scm);
            break;
        }

        if (svc)
            CloseServiceHandle(svc);

        CloseServiceHandle(scm);
        Sleep(500);
    }

    return true;
}
