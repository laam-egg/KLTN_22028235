#include <LightGBM/c_api.h>
#include <iostream>
#include <vector>
#include <filesystem>

#include "efe/core.h"
#include "mio/mmap.hpp"

template<typename T = feature_t>
constexpr int getLGBMInputDataType() {
    if constexpr (std::is_same_v<T, float>) {
        return C_API_DTYPE_FLOAT32;
    } else {
        static_assert( std::is_same_v<T, double> );
        return C_API_DTYPE_FLOAT64;
    }
}

double scan(BoosterHandle booster, char const* peFilePath) {
    auto absPath = std::filesystem::absolute(peFilePath);
    std::cerr << "Scanning: " << absPath << '\n';

    if (!std::filesystem::exists(peFilePath)) {
        std::cerr << "File does not exist: " << peFilePath << std::endl;
        return -1;
    } else {
        std::cerr << "File exists and size = "
                << std::filesystem::file_size(peFilePath)
                << " bytes\n";
    }

    // mmap it
    std::error_code error;
    mio::mmap_source mmap;
    try {
        mmap = mio::make_mmap_source(absPath.string(), 0, mio::map_entire_file, error);
    } catch (std::system_error const& e) {
        std::cerr << "Error while mmap'ing (exception): " << e.what() << '\n';
        return -1;
    }

    if (error) {
        std::cerr << "Error while mmap'ing: " << error << '\n';
        return -1;
    }
    std::cerr << "mmap'ed successfully" << '\n';

    #define N_ROWS 1
    #define N_COLS ef.getDim()

    EMBER2024FeatureExtractor ef;
    feature_t const* inputVector = ef.run(reinterpret_cast<uint8_t const*>(mmap.data()), mmap.size());
    std::cerr << "feature extraction run successfully" << '\n';
    // std::cerr << "feature vector:\n";
    // for (size_t i = 0; i < N_COLS; ++i) {
    //     if (i % 11 == 0) std::cerr << '\n';
    //     std::cerr << inputVector[i] << " ";
    // }
    // std::cerr << '\n';

    std::vector<double> out_result(N_ROWS, 0.0);
    int64_t out_len;

    const char* params = ""; // "predict_disable_shape_check=true";
    int ret = LGBM_BoosterPredictForMat(
        booster,
        inputVector,             // pointer to input features
        getLGBMInputDataType(),     // data type
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

    return out_result[0];
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <path/to/model/file> <path/to/executable>" << std::endl;
        return 1;
    }

    BoosterHandle booster;
    int num_iterations;

    char const* MODEL_FILE = argv[1];
    char const* PE_FILE = argv[2];

    // Load the pretrained model
    int ret = LGBM_BoosterCreateFromModelfile(MODEL_FILE, &num_iterations, &booster);
    if (ret != 0) {
        std::cerr << "Failed to load model!" << std::endl;
        return 1;
    }

    int num_features;
    LGBM_BoosterGetNumFeature(booster, &num_features);
    std::cout << "Model expects " << num_features << " features" << std::endl;

    double pred = scan(booster, PE_FILE);
    if (pred < 0) {
        std::cout << "Some error has occurred." << '\n';
    } else {
        std::cout << "Prediction: " << pred << std::endl;
    }

    // Free the booster
    LGBM_BoosterFree(booster);

    return 0;
}
