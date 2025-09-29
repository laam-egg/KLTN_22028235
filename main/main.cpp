#include <LightGBM/c_api.h>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path/to/model/file>" << std::endl;
        return 1;
    }

    #define N_ROWS 1
    #define N_COLS 2568
    BoosterHandle booster;
    int num_iterations;

    // Load the pretrained model
    int ret = LGBM_BoosterCreateFromModelfile(argv[1], &num_iterations, &booster);
    if (ret != 0) {
        std::cerr << "Failed to load model!" << std::endl;
        return 1;
    }

    int num_features;
    LGBM_BoosterGetNumFeature(booster, &num_features);
    std::cout << "Model expects " << num_features << " features" << std::endl;

    // Example input: 1 row
    std::vector<double> data(2568, 0.0);
    std::vector<double> out_result(N_ROWS, 0.0);
    int64_t out_len;

    const char* params = ""; // "predict_disable_shape_check=true";
    ret = LGBM_BoosterPredictForMat(
        booster,
        data.data(),             // pointer to input features
        C_API_DTYPE_FLOAT64,     // data type
        N_ROWS,                       // number of rows
        N_COLS,                       // number of columns
        1,                       // is_row_major
        C_API_PREDICT_NORMAL,    // prediction type
        0,                       // start_iteration
        -1,                      // num_iteration (-1 = all)
        params,                 // parameter string
        &out_len,
        out_result.data()
    );

    if (ret != 0) {
        const char *err_msg = LGBM_GetLastError();
        std::cerr << "Prediction failed: " << err_msg << std::endl;
        return 1;
    }

    std::cout << "Prediction: " << out_result[0] << std::endl;

    // Free the booster
    LGBM_BoosterFree(booster);

    return 0;
}
