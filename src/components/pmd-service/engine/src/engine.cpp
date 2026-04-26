#include "pmd_engine/engine.h"
#include "resources.h"
#include <windows.h>



PMDEngine::PMDEngine() {
    hMod = NULL;
    engineInstance = NULL;

    std::filesystem::path pmdEngineDLLPath = writePMDEngineDLLContent();
    pmdEngineDLLPath = pmdEngineDLLPath.wstring();
    wchar_t const* fullDLLPath = pmdEngineDLLPath.c_str();

    HANDLE hFile = CreateFileW(
        fullDLLPath,
        GENERIC_READ,
        FILE_SHARE_READ,        // deny WRITE and DELETE
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to open PMD Engine DLL file.");
    }

    // TODO: Verify integrity of the DLL file here

    hMod = LoadLibraryExW(
        fullDLLPath,
        NULL,
        // LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32
        0
    );
    CloseHandle(hFile);
    if (!hMod) {
        throw std::runtime_error("Failed to load PMD Engine DLL: " + std::to_string(GetLastError()));
    }

    // Load the symbols
    PMD_Engine_Init = reinterpret_cast<PMD_Engine_Init_Func>(GetProcAddress(static_cast<HMODULE>(hMod), "PMD_Engine_Init"));
    PMD_Engine_Predict = reinterpret_cast<PMD_Engine_Predict_Func>(GetProcAddress(static_cast<HMODULE>(hMod), "PMD_Engine_Predict"));
    PMD_Engine_Destroy = reinterpret_cast<PMD_Engine_Destroy_Func>(GetProcAddress(static_cast<HMODULE>(hMod), "PMD_Engine_Destroy"));

    if (!PMD_Engine_Init || !PMD_Engine_Predict || !PMD_Engine_Destroy) {
        FreeLibrary(static_cast<HMODULE>(hMod));
        throw std::runtime_error("Failed to get PMD Engine function addresses.");
    }

    // Initialize the engine
    engineInstance = PMD_Engine_Init();
    if (!engineInstance) {
        FreeLibrary(static_cast<HMODULE>(hMod));
        throw std::runtime_error("Failed to initialize PMD Engine instance.");
    }
}

PMDEngine::~PMDEngine() {
    if (engineInstance) {
        PMD_Engine_Destroy(engineInstance);
        engineInstance = NULL;
    }

    wchar_t const* fullDLLPath = pmdEngineDLLPath.c_str();

    if (hMod && hMod != INVALID_HANDLE_VALUE) {
        FreeLibrary(static_cast<HMODULE>(hMod));
        hMod = NULL;
    }
    
    // Delete the DLL (on close)
    DeleteFileW(fullDLLPath);
}

PMD_Engine_Decision PMDEngine::Predict(std::wstring const& filePath) {
    PMD_Engine_Decision decision = { -1.0, -1 };
    // if (!engineInstance) {
    // no need to do this
    //     throw std::runtime_error("PMD Engine instance is not initialized.");
    // }

    PMD_Engine_Predict(engineInstance, filePath.c_str(), &decision);
    return decision;
}
