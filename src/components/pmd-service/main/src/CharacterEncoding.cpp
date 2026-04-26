#include "CharacterEncoding.h"
#include <string>
#include <windows.h>

std::wstring from_utf8(std::string const& s)
{
    if (s.empty()) return L"";

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring result(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &result[0], size_needed);

    return result;
}

std::string to_utf8(std::wstring const& w)
{
    if (w.empty()) return "";

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &result[0], size_needed, nullptr, nullptr);

    return result;
}
