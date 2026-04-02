#include <iostream>
#include <efe/core.h>
#include <iostream>
#include <filesystem>
#include <chrono>

#include "ember2024_lgbm/model.h"

/**
 * Gemini
 */
#include <iomanip>
std::string formatFileSize(std::uintmax_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024 && unitIndex < 5) { // Up to Petabytes
        size /= 1024;
        unitIndex++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size << " " << units[unitIndex];
    return oss.str();
}

bool scanSingleFile(EMBER2024Model& model, EMBER2024FeatureExtractor& fe, std::filesystem::path const& filePath) {
    std::error_code errorCode;

    auto start = std::chrono::high_resolution_clock::now();
    feature_t const* featureVector = fe.run(filePath, errorCode);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    double elapsed_seconds = duration.count();
    double elapsed_ms = elapsed_seconds * 1000;

    std::string humanReadableFileSize = formatFileSize(std::filesystem::file_size(filePath));
    std::cerr << "FILE SIZE: " << humanReadableFileSize << '\n';

    std::cerr << "FEATURE EXTRACTION TIME: " << elapsed_ms << " milliseconds\n";

    if (errorCode) {
        std::cerr << "Error code value: " << errorCode.value() << "\n";
        std::cerr << "Error category: " << errorCode.category().name() << "\n";
        std::cerr << "Error message: " << errorCode.message() << "\n";
        return false;
    }

    size_t const dim = fe.getDim();

    std::vector<feature_t> featureVectorDouble(dim);
    for (size_t i = 0; i < dim; ++i) {
        featureVectorDouble[i] = static_cast<double>(featureVector[i]);
    }

    std::string inferenceError = "";
    start = std::chrono::high_resolution_clock::now();
    double pred = 0;
    try {
        pred = model.predict(featureVectorDouble.data(), dim);
    } catch (std::exception const& e) {
        inferenceError = e.what();
    }

    end = std::chrono::high_resolution_clock::now();
    duration = end - start;
    elapsed_seconds = duration.count();
    elapsed_ms = elapsed_seconds * 1000;
    std::cerr << "MODEL INFERENCE TIME: " << elapsed_ms << " milliseconds\n";

    if (inferenceError.empty()) {
        std::cout << "pred = " << pred << '\n';
    } else {
        std::cerr << "inference error: " << inferenceError << '\n';
    }

    return true;
}

int main(int argc, char** argv) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path/to/pe/file>" << std::endl;
        return 1;
    }
    std::filesystem::path filePath{ argv[1] };

    std::cerr << "Loading EMBER2024 Feature Extractor...";
    EMBER2024FeatureExtractor fe;
    std::cerr << "Done.\n";
    std::cerr << "Loading LightGBM model...";
    EMBER2024Model model;
    std::cerr << "Done.\n";
    std::cerr << "Scanning...\n";
    bool ok = scanSingleFile(model, fe, filePath);

    if (false == ok) {
        return 1;
    }

    return 0;
}
