#include "pmd_engine_v2.h"
#include "pmd_engine_v2_impl.h"

static_assert(
    sizeof(PMD2_Engine_Decision) == sizeof(PMD2Impl_Engine_Decision),
    "PMD2_Engine_Decision size mismatch"
);

#define BEGIN_EXTERN_C extern "C" {
#define END_EXTERN_C }

BEGIN_EXTERN_C

PMD_ENGINE_V2_API void* PMD2_Engine_Init(void) {
    return PMD2Impl_Engine_Init();
}

PMD_ENGINE_V2_API void PMD2_Engine_Destroy(IN void* engine) {
    PMD2Impl_Engine_Destroy(engine);
}

PMD_ENGINE_V2_API int PMD2_Engine_GetNumFeatures(
    IN void* engine,
    OUT size_t* numFeatures
) {
    return PMD2Impl_Engine_GetNumFeatures(
        engine,
        numFeatures
    );
}

PMD_ENGINE_V2_API int PMD2_Engine_ExtractFeatures32FromBuffer(
    IN void* engine,
    IN uint8_t const* buffer,
    IN size_t const bufferSize,
    OUT float** featureVectorPtr CALLEE_MANAGED,
    OUT size_t* featureVectorSize
) {
    return PMD2Impl_Engine_ExtractFeatures32FromBuffer(
        engine,
        buffer,
        bufferSize,
        featureVectorPtr,
        featureVectorSize
    );
}

PMD_ENGINE_V2_API int PMD2_Engine_ExtractFeatures32FromFilePathW(
    IN void* engine,
    IN wchar_t const* filePath,
    OUT float** featureVectorPtr CALLEE_MANAGED,
    OUT size_t* featureVectorSize
) {
    return PMD2Impl_Engine_ExtractFeatures32FromFilePathW(
        engine,
        filePath,
        featureVectorPtr,
        featureVectorSize
    );
}

PMD_ENGINE_V2_API int PMD2_Engine_PredictFromFeatures32(
    IN void* engine,
    IN float const* featureVector,
    IN size_t const featureVectorSize,
    OUT PMD2_Engine_Decision* decision
) {
    PMD2Impl_Engine_Decision d = { 0 };
    auto retval = PMD2Impl_Engine_PredictFromFeatures32(
        engine,
        featureVector,
        featureVectorSize,
        &d
    );
    if (decision != nullptr) {
        decision->score = d.score;
    }
    return retval;
}

PMD_ENGINE_V2_API int PMD2_Engine_PredictFromBuffer(
    IN void* engine,
    IN uint8_t const* buffer,
    IN size_t const bufferSize,
    OUT PMD2_Engine_Decision* decision
) {
    PMD2Impl_Engine_Decision d = { 0 };
    auto retval = PMD2Impl_Engine_PredictFromBuffer(
        engine,
        buffer,
        bufferSize,
        &d
    );
    if (decision != nullptr) {
        decision->score = d.score;
    }
    return retval;
}

PMD_ENGINE_V2_API int PMD2_Engine_PredictFromFilePathW(
    IN void* engine,
    IN wchar_t const* filePath,
    OUT PMD2_Engine_Decision* decision
) {
    PMD2Impl_Engine_Decision d = { 0 };
    auto retval = PMD2Impl_Engine_PredictFromFilePathW(
        engine,
        filePath,
        &d
    );
    if (decision != nullptr) {
        decision->score = d.score;
    }
    return retval;
}

END_EXTERN_C
