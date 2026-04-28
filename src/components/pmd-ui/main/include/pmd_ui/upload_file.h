#pragma once

#include <string>
#include <stdexcept>

std::string UploadFileLookup(
    const std::wstring& host,   // e.g. L"localhost"
    int port,         // e.g. 8000
    const std::wstring& filePath // local file to upload
) throw (std::runtime_error);
