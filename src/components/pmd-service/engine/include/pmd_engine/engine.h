#pragma once

#include <string>

extern "C" typedef struct _PMD_Engine_Decision {
    double score;
    int label;
} PMD_Engine_Decision;

extern "C" {
    // PMD_ENGINE_API void* PMD_Engine_Init(void);
    // PMD_ENGINE_API void PMD_Engine_Predict(IN void* engine, IN wchar_t const* filePath, OUT PMD_Engine_Decision* decision);
    // PMD_ENGINE_API void PMD_Engine_Destroy(IN void* engine);

    typedef void* (*PMD_Engine_Init_Func)(void);
    typedef void (*PMD_Engine_Predict_Func)(void* engine, wchar_t const* filePath, PMD_Engine_Decision* decision);
    typedef void (*PMD_Engine_Destroy_Func)(void* engine);
}

class PMDEngine {
public:
    PMDEngine();
    ~PMDEngine();

    PMD_Engine_Decision Predict(std::wstring const& filePath);

private:
    std::wstring pmdEngineDLLPath;
    void* hMod;
    void* engineInstance;
    PMD_Engine_Init_Func PMD_Engine_Init;
    PMD_Engine_Predict_Func PMD_Engine_Predict;
    PMD_Engine_Destroy_Func PMD_Engine_Destroy;
};
