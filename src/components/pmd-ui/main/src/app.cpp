#include <windows.h>
#include <string>

#include "pmd_ui/app.h"
#include "pmd_ui/main_view.h"

using namespace pmd_ui;

int App::Run(HINSTANCE hInstance, int nCmdShow)
{
    MainWindow win;
    if (!win.Create(hInstance, nCmdShow)) {
        MessageBoxW(nullptr, L"Could not create main window.", L"pmd-ui", MB_ICONERROR);
        return 1;
    }

    // Message loop
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
