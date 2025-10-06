#include "efe/core/f_exports_info.h"

#define DIM 129

#define EXPORTED_FUNCTIONS_FH_BUCKET_SIZE 128

class Offset {
public:
    #define ADD(NAME, VALUE) static constexpr size_t const NAME = VALUE;

    ADD(NUM_EXPORTED_FUNCTIONS, 0)
    ADD(EXPORTED_FUNCTIONS_HASHED, NUM_EXPORTED_FUNCTIONS + 1)

    #undef ADD

    static_assert(
        DIM == EXPORTED_FUNCTIONS_HASHED + EXPORTED_FUNCTIONS_FH_BUCKET_SIZE
    );
};

char const* ExportsInfo::getName() const {
    return "exports";
}

void ExportsInfo::reset(feature_t* output, PEFile const& peFile) {
    // pass
}

void ExportsInfo::start(feature_t* output, PEFile const& peFile) {
    std::memset(output, 0, DIM * sizeof(feature_t));
    if (!peFile.isPEFile() || !peFile.hasExportDirectory()) {
        return;
    }

    functionsFH.reset();
    functionsFH.setOutput(output + Offset::EXPORTED_FUNCTIONS_HASHED);
    functionsFH.setSize(EXPORTED_FUNCTIONS_FH_BUCKET_SIZE);
    functionsFH.start();

    std::string funcNameBuf;
    funcNameBuf.reserve(32);
    std::vector<ExportFunction> const& exportedFunctions = peFile.getExportedFunctions();
    for (ExportFunction const& func : exportedFunctions) {
        std::string const& funcName = func.name;
        if (!funcName.empty()) {
            functionsFH.reduce(funcName.substr(0, 10000).c_str(), 1.0);
        } else {
            funcNameBuf.assign("ordinal");
            funcNameBuf.append(std::to_string(func.ordinal));
            functionsFH.reduce(funcNameBuf.c_str(), 1.0);
        }
    }

    functionsFH.finalize();

    // size_t numExportedFunctions = exportedFunctions.size();
    // Due to: https://github.com/FutureComputing4AI/EMBER2024/issues/8
    constexpr size_t const numExportedFunctions = 128;

    output[Offset::NUM_EXPORTED_FUNCTIONS] = numExportedFunctions;
}
