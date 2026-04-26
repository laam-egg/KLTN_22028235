#include "pmd_ui/data_fetcher.h"
#include "pmd_ui/detection.h"
#include "pmd_ui/utils.h"
#include "pmd_ui/messages.h"
#include "pmd_driver/communication.h"

#include <memory>
#include <string>

using namespace pmd_ui;

static double PredScoreToDouble(const BYTE bytes[8])
{
    double d = 0.0;
    // The score is provided as raw 8 bytes; reinterpret safely
    static_assert(sizeof(double) == 8, "double must be 8 bytes");
    memcpy(&d, bytes, 8);
    // Clamp to [0,1] just in case
    if (d < 0.0) d = 0.0; else if (d > 1.0) d = 1.0;
    return d;
}

DataFetcher::DataFetcher(HWND notifyHwnd)
    : m_targetHwnd(notifyHwnd)
{
    m_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
}

DataFetcher::~DataFetcher()
{
    if (m_thread) CloseHandle(m_thread);
    if (m_stopEvent) CloseHandle(m_stopEvent);
}

bool DataFetcher::Start()
{
    m_thread = CreateThread(nullptr, 0, &DataFetcher::ThreadProc, this, 0, nullptr);
    return m_thread != nullptr;
}

void DataFetcher::StopAndJoin()
{
    if (m_stopEvent) SetEvent(m_stopEvent);
    if (m_thread) WaitForSingleObject(m_thread, INFINITE);
}

DWORD WINAPI DataFetcher::ThreadProc(LPVOID lpParameter)
{
    return reinterpret_cast<DataFetcher*>(lpParameter)->Run();
}

DWORD DataFetcher::Run()
{
    HANDLE hDev = OpenDriverDevice();
    if (hDev == INVALID_HANDLE_VALUE || hDev == nullptr) {
        // Notify disconnected
        PostMessageW(m_targetHwnd, WM_APP + 101, 0, 0);
        return 0;
    }

    // Poll every ~1s
    while (true) {
        // Check stop event with 1-second wait
        DWORD waitRes = WaitForSingleObject(m_stopEvent, 1000);
        if (waitRes == WAIT_OBJECT_0) break;

        BLOCK_OPERATION_DTO dto{};
        if (GetBlockOperation(hDev, &dto)) {
            auto det = std::make_unique<Detection>();
            det->detectionTime = dto.Timestamp;
            det->detectionTimeStr = FormatTimestamp(dto.Timestamp);
            det->fileId = dto.FileId;
            det->volumeSerialNumber = dto.VolumeSerialNumber;
            det->severity = PredScoreToDouble(dto.PredScore);
            det->isMalware = !dto.AllowExecution; // blocked => malware

            det->filePath = FileIdToPath(det->fileId, det->volumeSerialNumber);
            // Derive File Name
            det->fileName = det->filePath;
            size_t pos = det->fileName.find_last_of(L"\\/");
            if (pos != std::wstring::npos) det->fileName = det->fileName.substr(pos + 1);

            // Fallback if filename is empty or unavailable
            if (det->fileName.empty()) {
                wchar_t vsn[32]; swprintf(vsn, 32, L"0x%08X", det->volumeSerialNumber);
                det->fileName = L"VSN ";
                det->fileName += vsn;
                det->fileName += L" | FileId ";
                det->fileName += FormatFileId128Hex(det->fileId);
            }

            PostMessageW(m_targetHwnd, WM_APP_DETECTION, 0, reinterpret_cast<LPARAM>(det.release()));
        }
    }

    CloseDriverDevice(hDev);
    return 0;
}
