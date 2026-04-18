#pragma once

#include <string>
#include <windows.h>
#include <fileapi.h>

std::wstring FileIdToPath(const FILE_ID_128& fileId, ULONG volumeSerialNumber);

std::wstring FormatTimestamp(const LARGE_INTEGER& ts);

std::wstring FormatFileId128Hex(const FILE_ID_128& id);
