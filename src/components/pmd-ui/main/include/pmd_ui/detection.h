#pragma once

#include <string>
#include <windows.h>
#include "pmd_driver/config.h"

namespace pmd_ui {

struct Detection {
    std::wstring fileName;
    std::wstring filePath;

    LARGE_INTEGER detectionTime;
    std::wstring detectionTimeStr;

    FILE_ID_128 fileId;
    ULONG volumeSerialNumber;

    bool isMalware;
    double severity; // 0.0 - 1.0
};

}
