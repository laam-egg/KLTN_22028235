#include <efe/core.h>
#include <LightGBM/c_api.h>

template<typename T = feature_t>
constexpr int getLGBMInputDataType() {
    if constexpr (std::is_same_v<T, float>) {
        return C_API_DTYPE_FLOAT32;
    } else {
        static_assert( std::is_same_v<T, double> );
        return C_API_DTYPE_FLOAT64;
    }
}



#include "direct_model.h"

void* loadModel(const char* lgbmModelFileContent, int& numFeatures) {
    BoosterHandle boosterHandle = NULL;
    numFeatures = 0;

    int lgbmAPIReturnValue = 0;

    #define CHECK_LGBM_ERROR(call) \
        lgbmAPIReturnValue = call; \
        if (lgbmAPIReturnValue != 0) { \
                        if (boosterHandle != NULL) { \
                            LGBM_BoosterFree(boosterHandle); \
                        } \
            throw std::runtime_error(LGBM_GetLastError()); \
        } \
        (void)0 \
    
    int numIterations = 0;
    CHECK_LGBM_ERROR(LGBM_BoosterLoadModelFromString(lgbmModelFileContent, &numIterations, &boosterHandle));
    CHECK_LGBM_ERROR(LGBM_BoosterGetNumFeature(boosterHandle, &numFeatures));
    return (void*)boosterHandle;
}

double runInference(void* _booster, feature_t const* featureVector, size_t dim) {
    BoosterHandle booster = (BoosterHandle)_booster;

    #define N_ROWS 1
    #define N_COLS dim

    std::vector<double> out_result(N_ROWS, 0.0);
    int64_t out_len;

    const char* params = ""; // "predict_disable_shape_check=true";
    int ret = LGBM_BoosterPredictForMat(
        booster,
        featureVector,           // pointer to input features
        getLGBMInputDataType(),  // data type
        N_ROWS,                  // number of rows
        N_COLS,                  // number of columns
        1,                       // is_row_major
        C_API_PREDICT_NORMAL,    // prediction type
        0,                       // start_iteration
        -1,                      // num_iteration (-1 = all)
        params,                  // parameter string
        &out_len,
        out_result.data()
    );

    if (ret != 0) {
        const char *err_msg = LGBM_GetLastError();
        throw std::runtime_error(err_msg);
    }

    return out_result[0];

    #undef N_ROWS
    #undef N_COLS
}

void unloadModel(void* booster) {
    if (booster != NULL) {
        LGBM_BoosterFree((BoosterHandle)booster);
    }
}
