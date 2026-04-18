#include "resources.h"
#include <windows.h>
#include <random>
#include <fstream>

#define MAX_FILE_WRITE_RETRIES 20

class SecureRandom {
private:
    std::mt19937 generator;

public:
    SecureRandom() {
        std::random_device rd;
        generator.seed(rd());
    }

    int getInt(int min, int max) {
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(generator);
    }
};

int secureRandom() {
    static SecureRandom secureRandomInstance;
    return secureRandomInstance.getInt(12567, INT_MAX);
}

std::filesystem::path getExecutablePath() {
    std::vector<wchar_t> buffer(MAX_PATH);
    DWORD len = 0;

    while (true) {
        len = GetModuleFileNameW(NULL, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (len == 0) throw std::runtime_error("GetModuleFileNameW failed");
        if (len < buffer.size()) break; // success
        buffer.resize(buffer.size() * 2); // increase buffer
    }

    // Construct wstring with exact length
    std::wstring fullPath(buffer.data(), len);
    return std::filesystem::path(fullPath);
}

std::filesystem::path getExecutableDirectory() {
    auto path = getExecutablePath();
    auto parent = path.parent_path();
    if (parent.empty()) {
        // fallback in case of "." being returned
        parent = std::filesystem::current_path();
    }
    return parent;
}


std::filesystem::path writeDLLContent(
    std::filesystem::path const& dir,
    std::string const& dllName,
    bool randomize
) {
    auto fs = cmrc::engine_resources::get_filesystem();
    auto engineDLLFile = fs.open(dllName + ".dll");

    std::ofstream outFile;
    int retried = randomize ? MAX_FILE_WRITE_RETRIES : 1;
    std::filesystem::path tempPath;
    do {
        if (retried <= 0) {
            throw std::runtime_error("Failed to write PMD engine DLL file after multiple attempts.");
        }
        --retried;

        if (randomize) {
            // Sleep a bit to avoid collisions
            Sleep(10);
            tempPath = dir / (
                dllName + '_' + std::to_string(secureRandom()) + ".dll"
            );
        } else {
            tempPath = dir / (dllName + ".dll");
        }

        tempPath = std::filesystem::absolute(tempPath);
        outFile.open(tempPath, std::ios::binary | std::ios::out | std::ios::trunc);
    } while (!outFile.is_open());

    outFile.write(engineDLLFile.begin(), static_cast<std::streamsize>(engineDLLFile.size()));
    outFile.close();

    return tempPath;
}

std::filesystem::path writePMDEngineDLLContent() {
    std::filesystem::path p = getExecutableDirectory();

    writeDLLContent(p, "libcrypto-3-x64", false);
    writeDLLContent(p, "libssl-3-x64", false);
    return writeDLLContent(p, "pmd_engine", true);
}
