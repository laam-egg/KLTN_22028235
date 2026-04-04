#pragma once

#include <windows.h>

#ifdef PMD_ENGINE_EXPORTS
#define PMD_ENGINE_API __declspec(dllexport)
#else
#define PMD_ENGINE_API __declspec(dllimport)
#endif

extern "C" {
    typedef struct _PMD_Engine_Decision {
        double score;
        int label;
    } PMD_Engine_Decision;

    PMD_ENGINE_API void* PMD_Engine_Init(void);
    PMD_ENGINE_API int PMD_Engine_Predict(IN void* engine, IN wchar_t const* filePath, OUT PMD_Engine_Decision* decision);
    PMD_ENGINE_API void PMD_Engine_Destroy(IN void* engine);
}
