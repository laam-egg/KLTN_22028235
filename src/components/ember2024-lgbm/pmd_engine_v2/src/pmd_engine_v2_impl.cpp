#include "pmd_engine_v2_impl.h"
#include <efe/core.h>
#include "ember2024_lgbm/model.h"
#include <filesystem>


#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

extern "C" typedef struct _PMD2Impl_Engine {
    EMBER2024FeatureExtractor fe;
    EMBER2024Model model;
} PMD2Impl_Engine;

void* PMD2Impl_Engine_Init(void) {
    PMD2Impl_Engine* engine = new PMD2Impl_Engine{};
    return static_cast<void*>(engine);
}

void PMD2Impl_Engine_Destroy(IN void* engine) {
    delete static_cast<PMD2Impl_Engine*>(engine);
}



#define PROLOG(additional_condition) \
    if (!(engine != nullptr && (additional_condition))) { \
        return EXIT_FAILURE; \
    } \
    PMD2Impl_Engine* eng = static_cast<PMD2Impl_Engine*>(engine); \




int PMD2Impl_Engine_GetNumFeatures(IN void* engine, OUT size_t* numFeatures) {
    PROLOG(numFeatures != nullptr);
    *numFeatures = eng->fe.getDim();
    return EXIT_SUCCESS;
}

int PMD2Impl_Engine_ExtractFeatures32FromBuffer(
    void* engine,
    uint8_t const* buffer,
    size_t const bufferSize,
    OUT float** featureVectorPtr CALLEE_MANAGED,
    OUT size_t* featureVectorSize
) {
    PROLOG(
        buffer != nullptr && bufferSize > 0
        && featureVectorPtr != nullptr && featureVectorSize != nullptr
    );
    *featureVectorPtr = nullptr;
    *featureVectorSize = 0;
    static_assert(std::is_same_v<feature_t, float>);

    std::error_code errorCode;
    feature_t const* const features = eng->fe.run(buffer, bufferSize, errorCode);
    if (errorCode) {
        return EXIT_FAILURE;
    }
    *featureVectorPtr = const_cast<float*>(features);
    *featureVectorSize = eng->fe.getDim();
    return EXIT_SUCCESS;
}

int PMD2Impl_Engine_ExtractFeatures32FromFilePathW(
    void* engine,
    wchar_t const* filePath,
    OUT float** featureVectorPtr CALLEE_MANAGED,
    OUT size_t* featureVectorSize
) {
    PROLOG(
        filePath != nullptr
        && featureVectorPtr != nullptr && featureVectorSize != nullptr
    );
    *featureVectorPtr = nullptr;
    *featureVectorSize = 0;
    static_assert(std::is_same_v<feature_t, float>);

    std::error_code errorCode;
    feature_t const* const features = eng->fe.run(std::filesystem::path(filePath), errorCode);
    if (errorCode) {
        return EXIT_FAILURE;
    }
    *featureVectorPtr = const_cast<float*>(features);
    *featureVectorSize = eng->fe.getDim();
    return EXIT_SUCCESS;
}

int PMD2Impl_Engine_PredictFromFeatures32(
    IN void* engine,
    IN float const* featureVector,
    IN size_t const featureVectorSize,
    OUT PMD2Impl_Engine_Decision* decision
) {
    PROLOG(
        featureVector != nullptr && featureVectorSize > 0
        && decision != nullptr
    );
    memset(decision, 0, sizeof(PMD2Impl_Engine_Decision));
    try {
        double pred = eng->model.predict(featureVector, featureVectorSize);
        decision->score = pred;
        return EXIT_SUCCESS;
    } catch (...) {
        decision->score = -1.0;
        return EXIT_FAILURE;
    }
}

int PMD2Impl_Engine_PredictFromBuffer(
    void* engine,
    uint8_t const* buffer,
    size_t const bufferSize,
    PMD2Impl_Engine_Decision* decision
) {
    PROLOG(
        buffer != nullptr && bufferSize > 0
        && decision != nullptr
    );
    
    float* featureVector = nullptr;
    size_t featureVectorSize = 0;
    if (EXIT_SUCCESS != PMD2Impl_Engine_ExtractFeatures32FromBuffer(
        engine,
        buffer,
        bufferSize,
        &featureVector,
        &featureVectorSize
    )) {
        decision->score = -1.0;
        return EXIT_FAILURE;
    }

    return PMD2Impl_Engine_PredictFromFeatures32(
        engine,
        featureVector,
        featureVectorSize,
        decision
    );
}

int PMD2Impl_Engine_PredictFromFilePathW(
    IN void* engine,
    IN wchar_t const* filePath,
    OUT PMD2Impl_Engine_Decision* decision
) {
    PROLOG(
        filePath != nullptr
        && decision != nullptr
    );
    
    float* featureVector = nullptr;
    size_t featureVectorSize = 0;
    if (EXIT_SUCCESS != PMD2Impl_Engine_ExtractFeatures32FromFilePathW(
        engine,
        filePath,
        &featureVector,
        &featureVectorSize
    )) {
        decision->score = -1.0;
        return EXIT_FAILURE;
    }

    return PMD2Impl_Engine_PredictFromFeatures32(
        engine,
        featureVector,
        featureVectorSize,
        decision
    );
}
