#include "stdafx.h"
#include "ServiceInstaller.h"
#include "ServiceBase.h"
#include "WindowsServiceImpl.h"

// Internal name of the service
#define SERVICE_NAME             L"pmd-service"

// Displayed name of the service
#define SERVICE_DISPLAY_NAME     L"PE Malware Detection Service"

// Service start options.
#define SERVICE_START_TYPE       SERVICE_AUTO_START

// Start the service right after installation or not?
#define SERVICE_START_IMMEDIATELY  TRUE

// List of service dependencies - "dep1\0dep2\0\0"
#define SERVICE_DEPENDENCIES     L""

// The name of the account under which the service should run
#define SERVICE_ACCOUNT          NULL // LocalSystem

// The password to the service account name
#define SERVICE_PASSWORD         NULL



int wmain(int argc, wchar_t *argv[])
{
    if ((argc > 1) && ((*argv[1] == L'-' || (*argv[1] == L'/'))))
    {
        if (_wcsicmp(L"install", argv[1] + 1) == 0)
        {
            // Install the service when the command is 
            // "-install" or "/install".
			
            InstallOrReplaceService(
                SERVICE_NAME,               // Name of service
                SERVICE_DISPLAY_NAME,       // Name to display
                SERVICE_START_TYPE,         // Service start type
                SERVICE_DEPENDENCIES,       // Dependencies
                SERVICE_ACCOUNT,            // Service running account
                SERVICE_PASSWORD,           // Password of the account
                SERVICE_START_IMMEDIATELY   // Whether to start the service immediately after installation
                );
        }
        else if (_wcsicmp(L"remove", argv[1] + 1) == 0)
        {
            // Uninstall the service when the command is 
            // "-remove" or "/remove".
            UninstallServiceAndWait(SERVICE_NAME);
        }
        else
        {
            wprintf(L"Invalid command line argument: %s\n", argv[1]);
        }
    }
    else
    {
        wprintf(L"Parameters:\n");
        wprintf(L" -install  to install the service.\n");
        wprintf(L" -remove   to remove the service.\n");

        CWindowsServiceImpl service(SERVICE_NAME);
        if (!CServiceBase::Run(service))
        {
            wprintf(L"Service failed to run w/err 0x%08lx\n", GetLastError());
        }
    }

    return 0;
}