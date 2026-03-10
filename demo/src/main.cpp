#include <efe/core.h>
#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <cmdline_tools/argv.h>

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

bool scanSingleFile(EMBER2024FeatureExtractor& fe, std::filesystem::path const& filePath, bool silent) {
    std::error_code errorCode;

    auto start = std::chrono::high_resolution_clock::now();
    feature_t const* featureVector = fe.run(filePath, errorCode);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    double elapsed_seconds = duration.count();
    double elapsed_ms = elapsed_seconds * 1000;

    std::cerr << "FEATURE EXTRACTION TIME: " << elapsed_ms << " milliseconds\n";

    if (errorCode) {
        std::cerr << "Error code value: " << errorCode.value() << "\n";
        std::cerr << "Error category: " << errorCode.category().name() << "\n";
        std::cerr << "Error message: " << errorCode.message() << "\n";
        return false;
    }

    if (false == silent) {
        size_t const dim = fe.getDim();
        printFeatureVector(featureVector, dim);
    }
    return true;
}

int main(int argc, char** argv) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << "[FLAGS] <path/to/pe/file> [ANOTHER/PATH/TO/PE/FILE]..." << std::endl;
        return 1;
    }

    std::vector<std::wstring> args = getArgv(argc, argv);
    std::vector<std::filesystem::path> paths{};

    std::chrono::milliseconds time_gap{ 0 };
    bool silent = false;
    int loop = 1;

    enum flag_t {
        FLAG_NONE = 0,
        FLAG_TIME_GAP_MS = 1,
        FLAG_LOOP
    };
    flag_t flag = FLAG_NONE;
    for (size_t i = 1; i < args.size(); ++i) {
        std::wstring const& arg = args.at(i);
        if (flag == FLAG_NONE) {
            if (arg[0] == L'-' && arg[1] == L'-') {
                // long flag
                std::wstring_view flagName{ arg };
                flagName.remove_prefix(2);
                if (flagName == L"time-gap-ms") {
                    flag = FLAG_TIME_GAP_MS;
                } else if (flagName == L"silent") {
                    // boolean flag, enable only
                    silent = true;
                } else if (flagName == L"loop") {
                    flag = FLAG_LOOP;
                } else {
                    std::cerr << "Unknown flag\n";
                    return 1;
                }
            } else  {
                // path
                paths.push_back(arg);
            }
        } else {
            switch (flag)
            {
            case FLAG_TIME_GAP_MS:
                time_gap = std::chrono::milliseconds(std::stoi(arg));
                break;
            
            case FLAG_LOOP:
                loop = std::stoi(arg);
                break;
            
            default:
                break;
            }

            flag = FLAG_NONE;
        }
    }
    
    EMBER2024FeatureExtractor fe;

    bool first = true;
    for (int iLoop = 0; iLoop < loop; ++iLoop) {
        for (std::filesystem::path const& filePath : paths) {
            if (first) {
                first = false;
            } else {
                std::this_thread::sleep_for(time_gap);
            }
            std::cerr << "==========================\n";
            std::cerr << "Scanning file:\n";
            std::cerr << "    " << filePath.string() << '\n';
            std::cerr << "==========================\n";

            bool ok = scanSingleFile(fe, filePath, silent);
            if (false == ok) {
                return 1;
            }
        }
    }

    return 0;
}
