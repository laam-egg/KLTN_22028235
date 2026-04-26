#pragma once

#include <windows.h>
#include <string>
#include <sstream>

#include <io.h>
#include <fcntl.h>
#include <stdio.h>

#include "CharacterEncoding.h"

template<typename... Args>
void _DebugLog(const Args&... args)
{
    static bool consoleInitialized = false;
    if (!consoleInitialized) {
        _setmode(_fileno(stdout), _O_U16TEXT);
        _setmode(_fileno(stderr), _O_U16TEXT);
        consoleInitialized = true;
    }

    std::wostringstream oss;
    (oss << ... << args);
    oss << L'\n';

    OutputDebugStringW(oss.str().c_str());
    wprintf(L"%s", oss.str().c_str());
}

#ifdef DEBUGGING_RELATIVE_PATH
#define __FILENAME__ (&__FILE__[SOURCE_PATH_SIZE])
#else
#define __FILENAME__ __FILE__
#endif

#define DebugLog(...) _DebugLog( \
    L"File \"", \
    from_utf8(__FILENAME__), \
    L"\", line ", \
    __LINE__, \
    L": ", \
    __VA_ARGS__\
) \
