#include <efe/shared.h>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <chrono>

using feature_t = float;

void printFeatureVector(feature_t const* const featureVector, size_t const N) {
    constexpr size_t const LINE_BREAK_AT = 10;
    constexpr size_t const RANGE_IN_LINE = LINE_BREAK_AT - 1;
    
    size_t const MAX_INDEX = N - 1;

    for (size_t i = 0; i < N; ++i) {
        if (i % LINE_BREAK_AT == 0) {
            std::cout << '\n';
            std::cout << std::setw(4) << std::setfill('0') << i << '-';
            std::cout << std::setw(4) << std::setfill('0') << std::min(i + RANGE_IN_LINE, MAX_INDEX) << ": ";
        }
        std::cout << featureVector[i] << " ";
    }
    std::cout << "\n\nDIMENSION = " << N << '\n';
}

bool scanSingleFile(void* extractor, char const& filePath) {
    std::error_code errorCode;
    int result = 0;

    int dim = 0;
    result = EFE_GetNumFeatures(extractor, &dim);
    if (result != EXIT_SUCCESS) {
        std::cerr << "Failed to get number of features, code: " << result << "\n";
        return false;
    }
    
    float* featureVector = new float[dim];

    auto start = std::chrono::high_resolution_clock::now();
    result = EFE_ExtractFeatures32FromFileA(
        extractor,
        &filePath,
        featureVector,
        dim
    );
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    double elapsed_seconds = duration.count();
    double elapsed_ms = elapsed_seconds * 1000;

    std::cerr << "FEATURE EXTRACTION TIME: " << elapsed_ms << " milliseconds\n";

    if (result != EXIT_SUCCESS) {
        std::cerr << "Feature extraction failed with code: " << result << "\n";
    } else {
        printFeatureVector(featureVector, dim);
    }
    delete[] featureVector;

    return result == EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path/to/pe/file>" << std::endl;
        return 1;
    }

    char const* filePath = argv[1];

    void* extractor = EFE_Init();
    if (extractor == nullptr) {
        std::cerr << "Failed to initialize feature extractor." << std::endl;
        return 1;
    }

    bool ok = scanSingleFile(extractor, *filePath);
    EFE_Cleanup(extractor);

    if (false == ok) {
        return 1;
    }

    return 0;
}
