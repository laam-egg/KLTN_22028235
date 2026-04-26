#pragma region Includes
#include "WindowsServiceImpl.h"
#include "ThreadPool.h"
#include "Logging.h"
#include "utils.h"
#include "pmd_driver/driver.h"
#include "pmd_engine/engine.h"
#pragma endregion

CWindowsServiceImpl::CWindowsServiceImpl(PWSTR pszServiceName, 
                               BOOL fCanStop, 
                               BOOL fCanShutdown, 
                               BOOL fCanPauseContinue) : CServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
	m_fStopping = FALSE;

    // Create a manual-reset event that is not signaled at first to indicate 
    // the stopped signal of the service.
    m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hStoppedEvent == NULL)
    {
        throw GetLastError();
    }

    // Event to notify stopping to the worker thread
    m_hStoppingEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hStoppingEvent == NULL)
    {
        throw GetLastError();
    }
}


CWindowsServiceImpl::~CWindowsServiceImpl(void)
{
    if (m_hStoppingEvent)
    {
        CloseHandle(m_hStoppingEvent);
        m_hStoppingEvent = NULL;
    }

    if (m_hStoppedEvent)
    {
        CloseHandle(m_hStoppedEvent);
        m_hStoppedEvent = NULL;
    }
}

void CWindowsServiceImpl::OnStart(DWORD dwArgc, LPWSTR *lpszArgv)
{
    // Log a service start message to the Application log.
    WriteEventLogEntry(L"WindowsService in OnStart", 
        EVENTLOG_INFORMATION_TYPE);
    
    DebugLog(L"Service is starting...");

    // Queue the main service function for execution in a worker thread.
    CThreadPool::QueueUserWorkItem(&CWindowsServiceImpl::ServiceWorkerThread, this);
}

void CWindowsServiceImpl::ServiceWorkerThread(void)
{
    PENDING_SCAN_TASK pendingTask = { 0 };

    DebugLog(L"Service worker thread is running.");

    PMD_Engine_Decision decision = { 0 };
    std::unique_ptr<PMDEngine> pEngine;
    DebugLog(L"Opening pmd_engine...");
    try {
        pEngine = std::make_unique<PMDEngine>();
    } catch (std::exception& ex) {
        DebugLog(L"Failed to open pmd_engine: ", ex.what());
        goto cleanup;
    }
    DebugLog(L"... pmd_engine opened successfully.");

    DebugLog(L"Opening driver device...");
    HANDLE hDriver = OpenDriverDevice();
    while (hDriver == INVALID_HANDLE_VALUE || hDriver == NULL) {
        DebugLog(L"Failed to open driver device. Retrying in 10 seconds.");
        WaitForMultipleObjects(1, &m_hStoppingEvent, FALSE, 10000);
        if (m_fStopping) {
            DebugLog(L"Service worker thread is stopping during driver open retry.");
            goto cleanup;
        }
        HANDLE hDriver = OpenDriverDevice();
    }
    DebugLog(L"Driver device opened successfully.");

    if (!RegisterWithDriver(hDriver)) {
        DebugLog(L"Failed to register with driver. Exiting service worker thread.");
        goto cleanup;
    }

    HANDLE events[2] = { m_hStoppingEvent, NULL };
    while (!m_fStopping)
    {
        DebugLog(L"Calling GetPendingTaskAsync...");
        if (!GetPendingTaskAsync(hDriver, &pendingTask)) {
            DebugLog(L"GetPendingTaskAsync failed.");
            break;
        }
        events[1] = pendingTask.ov.hEvent;

        DebugLog(L"Waiting for either stopping event or pending task...");
        DWORD w = WaitForMultipleObjects(2, events, FALSE, INFINITE);

        DebugLog(L"Woke up from wait with result: ", w);
        if (w == WAIT_OBJECT_0) {
            CancelPendingTask(hDriver, &pendingTask);
            break;
        } else {
            SCAN_TASK_DTO task = { 0 };
            if (!CompletePendingTask(hDriver, &pendingTask, &task)) {
                DebugLog(L"CompletePendingTask failed.");
                break;
            }

            std::wstring filePath = FileIdToPath(task.FileId, task.VolumeSerialNumber);

            DebugLog(L"Scanning file: ", filePath, " ( PID: ", (ULONG_PTR)task.Pid, L" )");
            decision = pEngine->Predict(filePath);
            if (decision.score >= 0.0 && decision.label >= 0) {
                DebugLog(L"... Scan verdict: score = ", decision.score, L", label = ", decision.label);
                DebugLog(L"Reporting scan verdict to driver...");
                {
                    SCAN_VERDICT_DTO verdict = { 0 };
                    verdict.Version = 1;
                    verdict.VolumeSerialNumber = task.VolumeSerialNumber;
                    verdict.Pid = task.Pid;
                    memcpy(verdict.PredScore, &decision.score, sizeof(double));
                    verdict.FileId = task.FileId;
                    verdict.AllowExecution = (decision.label == 0); // allow if label is 0 (benign)

                    // TODO: With the current architecture,
                    // it is not even necessary to send the verdict back to the driver
                    // in case of AllowExecution = TRUE,
                    // as the driver will allow execution by default.
                    if (!SendVerdict(hDriver, &verdict)) {
                        DebugLog(L"... Failed to send verdict to driver.");
                        // break;
                    }
                }
            } else {
                DebugLog(L"... Scan result: Failed to get a valid decision from PMD engine.");
            }
        }
    }

    // Signal the stopped event.
cleanup:
    DebugLog(L"Service worker thread is stopping.");
    if (hDriver != INVALID_HANDLE_VALUE && hDriver != NULL) {
        CloseHandle(hDriver);
    }
    SetEvent(m_hStoppedEvent);
}

void CWindowsServiceImpl::OnStop()
{
    // Log a service stop message to the Application log.
    WriteEventLogEntry(L"WindowsService in OnStop", EVENTLOG_INFORMATION_TYPE);

    DebugLog(L"Service is stopping...");

    // Indicate that the service is stopping and wait for the finish of the 
    // main service function (ServiceWorkerThread).
    m_fStopping = TRUE;
    SetEvent(m_hStoppingEvent);
    if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0)
    {
        throw GetLastError();
    }

    DebugLog(L"Service has stopped.");
}
