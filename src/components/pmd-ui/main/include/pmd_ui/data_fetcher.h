#pragma once

#include <windows.h>
#include <string>

namespace pmd_ui {

class DataFetcher {
public:
    explicit DataFetcher(HWND notifyHwnd);
    ~DataFetcher();

    bool Start();
    void StopAndJoin();

private:
    static DWORD WINAPI ThreadProc(LPVOID lpParameter);
    DWORD Run();

private:
    HWND m_targetHwnd{nullptr};
    HANDLE m_stopEvent{nullptr};
    HANDLE m_thread{nullptr};
};

}
