#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>

#include "pmd_ui/detection.h"
#include "pmd_ui/messages.h"

namespace pmd_ui {

// Custom app messages declared in messages.h

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool Create(HINSTANCE hInstance, int nCmdShow);
    HWND hwnd() const { return m_hwnd; }

    void AddDetection(const Detection& d);
    void SetStatusConnectedTime(const std::wstring& when);
    void SetStatusDisconnected();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void InitControls();
    void CreateMenuBar();
    void CreateStatusBar();
    void CreateListView();
    void LayoutChildren();
    void EnsureCommonControls();

    void InsertListColumns();
    void UpdateListViewRow(int index, const Detection& d);
    void ShowDetailDialog(int index);
    void ShowNotification(const Detection& d);
    void SortByColumn(int col);

private:
    HWND m_hwnd{nullptr};
    HWND m_list{nullptr};
    HWND m_status{nullptr};

    std::vector<Detection> m_items;
    int m_sortCol{0};
    bool m_sortAsc{true};
};

}
