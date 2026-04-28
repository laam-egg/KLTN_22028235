#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <fstream>
#pragma comment(lib, "winhttp.lib")

#include <nlohmann/json.hpp>
using json = nlohmann::json;

static std::vector<BYTE> ReadFileBytes(const std::wstring& path) throw (std::runtime_error) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) throw std::runtime_error("Cannot open file");
    auto size = f.tellg();
    f.seekg(0);
    std::vector<BYTE> buf(size);
    f.read(reinterpret_cast<char*>(buf.data()), size);
    return buf;
}

std::string UploadFileLookup(
    const std::wstring& host,   // e.g. L"localhost"
    int /*INTERNET_PORT*/ port,         // e.g. 8000
    const std::wstring& filePath // local file to upload
) throw (std::runtime_error) {
    // --- Build multipart body ---
    std::string boundary = "----WinHTTPBoundary7MA4YWxkTrZu0gW";
    std::string filename = "sample.exe"; // or derive from filePath - now just use this generic name

    // Read file bytes
    auto fileBytes = ReadFileBytes(filePath);

    // Build body: preamble + file bytes + epilogue
    std::string preamble =
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"" + filename + "\"\r\n"
        "Content-Type: application/octet-stream\r\n\r\n";

    std::string epilogue = "\r\n--" + boundary + "--\r\n";

    // Total body = preamble + fileBytes + epilogue
    std::vector<BYTE> body;
    body.insert(body.end(), preamble.begin(), preamble.end());
    body.insert(body.end(), fileBytes.begin(), fileBytes.end());
    body.insert(body.end(), epilogue.begin(), epilogue.end());

    // Content-Type header
    std::string contentType =
        "Content-Type: multipart/form-data; boundary=" + boundary;

    // --- WinHTTP setup ---
    HINTERNET hSession = WinHttpOpen(
        L"pmd-ui/1.0", // user agent
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!hSession) {
        throw std::runtime_error("Cannot open WinHTTP session");
    }

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        L"/lookup",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE); // 0 if HTTP e.g. localhost

    // Add Content-Type header
    std::wstring wContentType(contentType.begin(), contentType.end());
    WinHttpAddRequestHeaders(
        hRequest,
        wContentType.c_str(),
        (DWORD)-1,
        WINHTTP_ADDREQ_FLAG_ADD);

    // Send with body size known upfront
    BOOL ok = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0,
        (DWORD)body.size(), // total content length
        0);

    if (ok) {
        DWORD written = 0;
        ok = WinHttpWriteData(hRequest, body.data(), (DWORD)body.size(), &written);
    }

    if (ok) ok = WinHttpReceiveResponse(hRequest, NULL);

    // Read response
    std::string result;
    if (ok) {
        std::string response;
        DWORD bytesAvail = 0;
        while (WinHttpQueryDataAvailable(hRequest, &bytesAvail) && bytesAvail > 0) {
            std::vector<char> buf(bytesAvail + 1, 0);
            DWORD bytesRead = 0;
            WinHttpReadData(hRequest, buf.data(), bytesAvail, &bytesRead);
            response.append(buf.data(), bytesRead);
        }
        // response is JSON: {"sha256": "...", "url": "..."}
        auto data = json::parse(response);
        result = data["url"].get<std::string>();
    } else {
        throw std::runtime_error("Failed to send request or receive response");
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;
}
