#include "cmdline_tools/argv.h"

#include <vector>
#include <string>
#include <stdexcept>

#ifdef _WIN32

#include <windows.h>
#include <shellapi.h>

std::vector<std::wstring> getArgv(int /*argc*/, char** /*argv*/)
{
    int wargc = 0;
    wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    if (!wargv)
        throw std::runtime_error("CommandLineToArgvW failed");

    std::vector<std::wstring> result;
    result.reserve(wargc);

    for (int i = 0; i < wargc; ++i)
        result.emplace_back(wargv[i]);

    LocalFree(wargv);
    return result;
}

#else  // POSIX

#include <iconv.h>
#include <cstring>
#include <cerrno>

static std::wstring utf8_to_wstring(const char* s)
{
    iconv_t cd = iconv_open("WCHAR_T", "UTF-8");
    if (cd == (iconv_t)-1)
        throw std::runtime_error("iconv_open failed");

    size_t inbytes = std::strlen(s);
    size_t outbytes = (inbytes + 1) * sizeof(wchar_t);

    std::wstring out;
    out.resize(outbytes / sizeof(wchar_t));

    char* inbuf = const_cast<char*>(s);
    char* outbuf = reinterpret_cast<char*>(&out[0]);

    size_t res = iconv(cd, &inbuf, &inbytes, &outbuf, &outbytes);
    iconv_close(cd);

    if (res == (size_t)-1)
        throw std::runtime_error("iconv UTF-8 → wchar_t failed");

    out.resize(
        reinterpret_cast<wchar_t*>(outbuf) - out.data()
    );
    return out;
}

std::vector<std::wstring> getArgv(int argc, char** argv)
{
    std::vector<std::wstring> result;
    result.reserve(argc);

    for (int i = 0; i < argc; ++i)
        result.emplace_back(utf8_to_wstring(argv[i]));

    return result;
}

#endif
