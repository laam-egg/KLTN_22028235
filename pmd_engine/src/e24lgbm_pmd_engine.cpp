#include "e24lgbm_pmd_engine.h"
#include "e24lgbm_pmd_engine_impl.h"

extern "C"
PMD_ENGINE_API void* PMD_Engine_Init(void) {
    return PMDImpl_Engine_Init();
}

extern "C"
PMD_ENGINE_API void PMD_Engine_Destroy(IN void* engine) {
    PMDImpl_Engine_Destroy(engine);
}

extern "C"
PMD_ENGINE_API int PMD_Engine_Predict(
    IN void* engine,
    IN wchar_t const* filePath,
    OUT PMD_Engine_Decision* decision
) {
    PMDImpl_Engine_Decision d = { 0 };
    auto returnValue = PMDImpl_Engine_Predict(
        engine,
        filePath,
        &d
    );
    if (decision != nullptr) {
        decision->score = d.score;
        decision->label = d.label;
    }
    return returnValue;
}
