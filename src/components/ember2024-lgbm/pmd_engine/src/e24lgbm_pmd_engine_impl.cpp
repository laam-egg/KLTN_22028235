#include "e24lgbm_pmd_engine_impl.h"
#include <efe/core.h>
#include "ember2024_lgbm/model.h"
#define THRESHOLD 0.8
#define FALSE 0
#define TRUE 1

extern "C" {

typedef struct _PMDImpl_Engine {
    EMBER2024FeatureExtractor fe;
    EMBER2024Model model;
} PMDImpl_Engine;

void* PMDImpl_Engine_Init(void) {
    PMDImpl_Engine* engine = new PMDImpl_Engine();
    return static_cast<void*>(engine);
}

void PMDImpl_Engine_Destroy(void* engine) {
    delete static_cast<PMDImpl_Engine*>(engine);
}

int PMDImpl_Engine_Predict(
    void* engine,
    const wchar_t* filePath,
    PMDImpl_Engine_Decision* decision
) {
    if (decision == nullptr || engine == nullptr || filePath == nullptr) {
        return FALSE;
    }
    memset(decision, 0, sizeof(PMDImpl_Engine_Decision));

    PMDImpl_Engine* eng = static_cast<PMDImpl_Engine*>(engine);

    std::error_code errorCode;
    feature_t const* featureVector = eng->fe.run(std::filesystem::path(filePath), errorCode);

    if (errorCode) {
        decision->score = -1.0;
        decision->label = -1;
        return FALSE;
    }

    size_t const dim = eng->fe.getDim();
    try {
        double pred = eng->model.predict(featureVector, dim);
        decision->score = pred;
        decision->label = (pred >= THRESHOLD) ? 1 : 0;
        return TRUE;
    } catch (...) {
        decision->score = -1.0;
        decision->label = -1;
        return FALSE;
    }
}

} // extern "C"
