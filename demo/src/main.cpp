#include <efe/core.h>
#include <iostream>
#include <filesystem>

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

bool scanSingleFile(EMBER2024FeatureExtractor& fe, std::filesystem::path const& filePath) {
    std::error_code errorCode;
    feature_t const* featureVector = fe.run(filePath, errorCode);
    if (errorCode) {
        std::cerr << "Error code value: " << errorCode.value() << "\n";
        std::cerr << "Error category: " << errorCode.category().name() << "\n";
        std::cerr << "Error message: " << errorCode.message() << "\n";
        return false;
    }

    size_t const dim = fe.getDim();
    printFeatureVector(featureVector, dim);
    return true;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path/to/pe/file>" << std::endl;
        return 1;
    }
    std::filesystem::path filePath{ argv[1] };

    EMBER2024FeatureExtractor fe;
    bool ok = scanSingleFile(fe, filePath);

    if (false == ok) {
        return 1;
    }

    return 0;
}
