#include "pmd_ui/main_view.h"
#include "pmd_ui/detection.h"
#include "pmd_ui/data_fetcher.h"
#include "pmd_ui/utils.h"
#include "pmd_ui/upload_file.h"
#include "pmd_driver/communication.h"

#include <commctrl.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>

using namespace pmd_ui;

namespace {
    const wchar_t kClassName[] = L"pmd_ui_main_window";
    LRESULT CALLBACK NotificationProc(HWND h, UINT m, WPARAM w, LPARAM l) {
        switch (m) {
        case WM_TIMER: {
            KillTimer(h, 1); DestroyWindow(h); return 0;
        }
        case WM_LBUTTONUP: {
            DestroyWindow(h);
            // Bring main window to front
            HWND owner = GetWindow(h, GW_OWNER);
            if (owner) { ShowWindow(owner, SW_SHOWNORMAL); SetForegroundWindow(owner); }
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(h, &ps);
            RECT rc; GetClientRect(h, &rc);
            HBRUSH hbr = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(hdc, &rc, hbr);
            DeleteObject(hbr);

            auto textPtr = reinterpret_cast<const std::wstring*>(GetWindowLongPtrW(h, GWLP_USERDATA));
            if (textPtr) {
                HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
                HFONT old = (HFONT)SelectObject(hdc, hFont);
                SetBkMode(hdc, TRANSPARENT);
                DrawTextW(hdc, textPtr->c_str(), -1, &rc, DT_LEFT | DT_TOP | DT_NOPREFIX | DT_WORDBREAK);
                SelectObject(hdc, old);
            }
            EndPaint(h, &ps);
            return 0;
        }
        case WM_NCDESTROY: {
            auto textPtr = reinterpret_cast<std::wstring*>(GetWindowLongPtrW(h, GWLP_USERDATA));
            if (textPtr) { delete textPtr; SetWindowLongPtrW(h, GWLP_USERDATA, 0); }
            break;
        }
        }
        return DefWindowProcW(h, m, w, l);
    }
}

MainWindow::MainWindow() {}
MainWindow::~MainWindow() {}

void MainWindow::EnsureCommonControls()
{
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);
}

bool MainWindow::Create(HINSTANCE hInstance, int nCmdShow)
{
    EnsureCommonControls();

    WNDCLASSW wc{};
    wc.lpfnWndProc = &MainWindow::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kClassName;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

    if (!RegisterClassW(&wc)) {
        return false;
    }

    m_hwnd = CreateWindowExW(
        0, kClassName, L"pmd-ui",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 600,
        nullptr, nullptr, hInstance, this);

    if (!m_hwnd) return false;

    CreateMenuBar();
    InitControls();
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);

    // Start worker
    auto fetcher = new DataFetcher(m_hwnd);
    if (!fetcher->Start()) {
        SetStatusDisconnected();
        delete fetcher;
    } else {
        // Store pointer in window property for cleanup
        SetPropW(m_hwnd, L"pmd_ui_fetcher", reinterpret_cast<HANDLE>(fetcher));
    }

    return true;
}

void MainWindow::InitControls()
{
    CreateStatusBar();
    CreateListView();
    InsertListColumns();
    LayoutChildren();
}

void MainWindow::CreateMenuBar()
{
    HMENU hMenuBar = CreateMenu();
    HMENU hFile = CreateMenu();
    HMENU hHelp = CreateMenu();

    AppendMenuW(hFile, MF_STRING, 1001, L"Exit");
    AppendMenuW(hHelp, MF_STRING, 2001, L"About");

    AppendMenuW(hMenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(hFile), L"File");
    AppendMenuW(hMenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(hHelp), L"Help");
    SetMenu(m_hwnd, hMenuBar);
}

void MainWindow::CreateStatusBar()
{
    m_status = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr,
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, m_hwnd, reinterpret_cast<HMENU>(1),
        reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(m_hwnd, GWLP_HINSTANCE)), nullptr);
    SetStatusDisconnected();
}

void MainWindow::CreateListView()
{
    m_list = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        0, 0, 0, 0, m_hwnd, reinterpret_cast<HMENU>(2),
        reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(m_hwnd, GWLP_HINSTANCE)), nullptr);

    ListView_SetExtendedListViewStyle(m_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
}

void MainWindow::InsertListColumns()
{
    LVCOLUMNW col{}; col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    col.pszText = const_cast<LPWSTR>(L"File Name"); col.cx = 280; col.iSubItem = 0; ListView_InsertColumn(m_list, 0, &col);
    col.pszText = const_cast<LPWSTR>(L"Detection Time"); col.cx = 180; col.iSubItem = 1; ListView_InsertColumn(m_list, 1, &col);
    col.pszText = const_cast<LPWSTR>(L"Label"); col.cx = 120; col.iSubItem = 2; ListView_InsertColumn(m_list, 2, &col);
    col.pszText = const_cast<LPWSTR>(L"Severity"); col.cx = 120; col.iSubItem = 3; ListView_InsertColumn(m_list, 3, &col);
}

void MainWindow::LayoutChildren()
{
    RECT rc{}; GetClientRect(m_hwnd, &rc);
    RECT rs{}; SendMessageW(m_status, WM_SIZE, 0, 0); GetWindowRect(m_status, &rs);
    int statusHeight = rs.bottom - rs.top;
    MoveWindow(m_list, 0, 0, rc.right - rc.left, rc.bottom - rc.top - statusHeight, TRUE);
}

void MainWindow::AddDetection(const Detection& d)
{
    int idx = static_cast<int>(m_items.size());
    m_items.push_back(d);

    LVITEMW item{}; item.mask = LVIF_TEXT | LVIF_STATE; item.iItem = idx; item.iSubItem = 0;
    item.pszText = const_cast<LPWSTR>(d.fileName.c_str());
    ListView_InsertItem(m_list, &item);
    UpdateListViewRow(idx, d);
    ShowNotification(d);

    SetStatusConnectedTime(d.detectionTimeStr);
}

void MainWindow::UpdateListViewRow(int index, const Detection& d)
{
    ListView_SetItemText(m_list, index, 1, const_cast<LPWSTR>(d.detectionTimeStr.c_str()));
    ListView_SetItemText(m_list, index, 2, const_cast<LPWSTR>(d.isMalware ? L"malware" : L"benign"));

    wchar_t sevBuf[32];
    swprintf(sevBuf, 32, L"%.3f", d.severity);
    ListView_SetItemText(m_list, index, 3, sevBuf);
}

void MainWindow::SetStatusConnectedTime(const std::wstring& when)
{
    std::wstring msg = L"Last updated at " + when;
    SendMessageW(m_status, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(msg.c_str()));
}

void MainWindow::SetStatusDisconnected()
{
    SendMessageW(m_status, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(L"Could not connect to pmd-driver."));
}

void MainWindow::ShowDetailDialog(int index)
{
    if (index < 0 || index >= static_cast<int>(m_items.size())) return;
    const Detection& d = m_items[index];

    std::wstring text;
    text.reserve(512);
    text += L"Detection Time: " + d.detectionTimeStr + L"\n";
    text += L"File Path: " + d.filePath + L"\n";
    text += L"File ID: " + FormatFileId128Hex(d.fileId) + L"\n";
    wchar_t vsn[32]; swprintf(vsn, 32, L"0x%08X", d.volumeSerialNumber);
    text += L"Volume Serial Number: "; text += vsn; text += L"\n";
    text += L"Label: "; text += (d.isMalware ? L"malware" : L"benign"); text += L"\n";
    wchar_t sev[32]; swprintf(sev, 32, L"%.3f", d.severity);
    text += L"Severity: "; text += sev;

    text += L"\n\nDo you want to submit this sample onto Deep Analysis for further inspection?";

    int answer = MessageBoxW(m_hwnd, text.c_str(), L"Detection Details", MB_YESNO | MB_ICONINFORMATION);

    if (IDYES == answer) {
        try {
            std::string url = UploadFileLookup(
                L"pmd-deep-analysis.vutunglam.id.vn",
                443,
                d.filePath
            );
            ShellExecuteA(m_hwnd, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        } catch (std::exception const& ex) {
            MessageBoxW(m_hwnd, std::wstring(L"Failed to submit file: " + std::wstring(ex.what(), ex.what() + strlen(ex.what()))).c_str(), L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::ShowNotification(const Detection& d)
{
    // Simple popup near bottom-right
    RECT work{}; SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int width = 300, height = 90;
    int x = work.right - width - 10;
    int y = work.bottom - height - 10;

    HWND popup = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"STATIC", L"",
        WS_POPUP | WS_BORDER,
        x, y, width, height,
        m_hwnd, nullptr, reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(m_hwnd, GWLP_HINSTANCE)), nullptr);

    std::wstring msg = L"Detected: " + d.fileName + L"\nSeverity: ";
    wchar_t sev[32]; swprintf(sev, 32, L"%.3f", d.severity);
    msg += sev;

    // Store text for painting
    auto* textHeap = new std::wstring(msg);
    SetWindowLongPtrW(popup, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(textHeap));
    ShowWindow(popup, SW_SHOWNOACTIVATE);

    // Auto close after 3 seconds
    SetTimer(popup, 1, 3000, nullptr);

    // Subclass with a simple static proc to draw text and handle dismissal
    SetWindowLongPtrW(popup, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&NotificationProc));
}

void MainWindow::SortByColumn(int col)
{
    if (m_sortCol == col) m_sortAsc = !m_sortAsc; else { m_sortCol = col; m_sortAsc = true; }

    auto cmp = [&](const Detection& a, const Detection& b) {
        int sign = m_sortAsc ? 1 : -1;
        switch (m_sortCol) {
        case 0: return sign * (int)_wcsicmp(a.fileName.c_str(), b.fileName.c_str()) < 0;
        case 1: {
            if (a.detectionTime.QuadPart == b.detectionTime.QuadPart) return false;
            return m_sortAsc ? (a.detectionTime.QuadPart < b.detectionTime.QuadPart) : (a.detectionTime.QuadPart > b.detectionTime.QuadPart);
        }
        case 2: {
            int la = a.isMalware ? 1 : 0;
            int lb = b.isMalware ? 1 : 0;
            return m_sortAsc ? (la < lb) : (la > lb);
        }
        case 3: return m_sortAsc ? (a.severity < b.severity) : (a.severity > b.severity);
        }
        return false;
    };

    std::stable_sort(m_items.begin(), m_items.end(), cmp);

    // Refresh list
    ListView_DeleteAllItems(m_list);
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        const auto& d = m_items[i];
        LVITEMW item{}; item.mask = LVIF_TEXT | LVIF_STATE; item.iItem = i; item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(d.fileName.c_str());
        ListView_InsertItem(m_list, &item);
        UpdateListViewRow(i, d);
    }
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MainWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) return self->HandleMessage(msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_SIZE:
        LayoutChildren();
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1001: // Exit
            PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
            return 0;
        case 2001: // About
            MessageBoxW(m_hwnd, L"pmd-ui\nVersion: 1.0.0\nAuthor: Vu Tung Lam", L"About", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        break;
    case WM_NOTIFY:
        if (reinterpret_cast<NMHDR*>(lParam)->hwndFrom == m_list) {
            auto code = reinterpret_cast<NMHDR*>(lParam)->code;
            if (code == NM_DBLCLK) {
                int sel = ListView_GetNextItem(m_list, -1, LVNI_SELECTED);
                if (sel != -1) ShowDetailDialog(sel);
                return 0;
            }
            if (code == LVN_COLUMNCLICK) {
                auto* nm = reinterpret_cast<NMLISTVIEW*>(lParam);
                SortByColumn(nm->iSubItem);
                return 0;
            }
        }
        break;
    case WM_APP_DETECTION: {
        // LPARAM carries pointer to Detection allocated by worker
        std::unique_ptr<Detection> d(reinterpret_cast<Detection*>(lParam));
        if (d) AddDetection(*d);
        return 0;
    }
    case WM_CLOSE: {
        // Stop worker
        auto fetcher = reinterpret_cast<DataFetcher*>(GetPropW(m_hwnd, L"pmd_ui_fetcher"));
        if (fetcher) {
            fetcher->StopAndJoin();
            RemovePropW(m_hwnd, L"pmd_ui_fetcher");
            delete fetcher;
        }
        DestroyWindow(m_hwnd);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(m_hwnd, msg, wParam, lParam);
}
