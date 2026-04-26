/**
 * ATTENTION: Example code snippets below are simplified.
 * One should always check the return values of those
 * function calls. Every function returning an `int`
 * would return either `EXIT_SUCCESS` (0) or any non-zero
 * value on failure (maybe 1, maybe 2, etc.).
 * Functions returning pointers would return `nullptr` on failure.
 * Functions returning void are assumed to always succeed.
 * 
 * =====================================================================================
 * 
 * Simplest use case: just predict from file path:
 * 
 * ```c
 * void* engine = PMD2_Engine_Init();
 * PMD2_Engine_Decision decision = { 0 };
 * int result = PMD2_Engine_PredictFromFilePathW(engine, L"sample.exe", &decision);
 * // use decision.score and decision.label
 * PMD2_Engine_Destroy(engine);
 * ```
 * 
 * Similarly, use `PMD2_Engine_PredictFromBuffer` to predict from a memory buffer
 * instead.
 * 
 * More advanced use case: extract features from a file or buffer, then predict:
 * 
 * ```cpp
 * void* engine = PMD2_Engine_Init();
 * 
 * size_t numFeatures = 0;
 * PMD2_Engine_GetNumFeatures(engine, &numFeatures);
 * 
 * float* features = nullptr;
 * size_t featureSize = 0;
 * PMD2_Engine_ExtractFeatures32FromFilePathW(
 *     engine,
 *     L"sample.exe",
 *     &features,
 *     &featureSize
 * );
 * 
 * PMD2_Engine_Decision decision = { 0 };
 * int predictResult = PMD2_Engine_PredictFromFeatures32(
 *     engine,
 *     features,
 *     featureSize,
 *     &decision
 * );
 * 
 * // use decision.score and decision.label
 * 
 * PMD2_Engine_Destroy(engine);
 * ```
 * 
 * =====================================================================================
 * 
 */

#pragma once

#include <windows.h>
#include <stddef.h>
#include <stdint.h>



#ifdef PMD_ENGINE_V2_EXPORTS
#define PMD_ENGINE_V2_API __declspec(dllexport)
#else
#define PMD_ENGINE_V2_API __declspec(dllimport)
#endif



#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

/**
 * Pointers retrieved via a parameter
 * denoted with this macro are managed
 * by the callee (the function being called).
 * Therefore, the caller must NOT free
 * or delete such pointers.
 */
#ifndef CALLEE_MANAGED
#define CALLEE_MANAGED
#endif



extern "C" {
    typedef struct _PMD2_Engine_Decision {
        double score;
    } PMD2_Engine_Decision;

    PMD_ENGINE_V2_API void* PMD2_Engine_Init(void);

    PMD_ENGINE_V2_API int PMD2_Engine_GetNumFeatures(IN void* engine, OUT size_t* numFeatures);

    PMD_ENGINE_V2_API int PMD2_Engine_ExtractFeatures32FromBuffer(
        IN void* engine,
        IN uint8_t const* buffer,
        IN size_t const bufferSize,
        OUT float** featureVectorPtr CALLEE_MANAGED,
        OUT size_t* featureVectorSize
    );

    PMD_ENGINE_V2_API int PMD2_Engine_ExtractFeatures32FromFilePathW(
        IN void* engine,
        IN wchar_t const* filePath,
        OUT float** featureVectorPtr CALLEE_MANAGED,
        OUT size_t* featureVectorSize
    );

    PMD_ENGINE_V2_API int PMD2_Engine_PredictFromFeatures32(
        IN void* engine,
        IN float const* featureVector,
        IN size_t const featureVectorSize,
        OUT PMD2_Engine_Decision* decision
    );

    PMD_ENGINE_V2_API int PMD2_Engine_PredictFromBuffer(
        IN void* engine,
        IN uint8_t const* buffer,
        IN size_t const bufferSize,
        OUT PMD2_Engine_Decision* decision
    );

    PMD_ENGINE_V2_API int PMD2_Engine_PredictFromFilePathW(
        IN void* engine,
        IN wchar_t const* filePath,
        OUT PMD2_Engine_Decision* decision
    );

    PMD_ENGINE_V2_API void PMD2_Engine_Destroy(IN void* engine);
}
