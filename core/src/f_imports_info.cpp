#include "efe/core/f_imports_info.h"
#include <string>

#define DIM 1282

#define LIBRARIES_FH_BUCKET_SIZE 256
#define FUNCTIONS_FH_BUCKET_SIZE 1024

class Offset {
public:
    #define ADD(NAME, VALUE) static constexpr size_t const NAME = VALUE;

    ADD(NUM_FUNCTIONS, 0) // full function names e.g. kernel32.dll:CreateFileMappingA
    ADD(NUM_LIBRARIES, NUM_FUNCTIONS + 1)
    ADD(LIBRARIES_HASHED, NUM_LIBRARIES + 1)
    ADD(FUNCTIONS_HASHED, LIBRARIES_HASHED + LIBRARIES_FH_BUCKET_SIZE)

    static_assert(
        FUNCTIONS_HASHED + FUNCTIONS_FH_BUCKET_SIZE == DIM
    );

    #undef ADD
};

char const* ImportsInfo::getName() const {
    return "imports";
}

void ImportsInfo::reset(feature_t* output, PEFile const& peFile) {
    // pass
}

void ImportsInfo::start(feature_t* output, PEFile const& peFile) {
    std::memset(output, 0, DIM * sizeof(feature_t));
    if (!peFile.isPEFile() || !peFile.hasImportDirectory()) {
        return;
    }

    librariesFH.reset();
    librariesFH.setOutput(output + Offset::LIBRARIES_HASHED);
    librariesFH.setSize(LIBRARIES_FH_BUCKET_SIZE);
    librariesFH.start();

    functionsFH.reset();
    functionsFH.setOutput(output + Offset::FUNCTIONS_HASHED);
    functionsFH.setSize(FUNCTIONS_FH_BUCKET_SIZE);
    functionsFH.start();

    std::vector<ImportLibrary> const& libraries = peFile.getImportLibraries();
    size_t const numLibraries = libraries.size();
    size_t numFunctions = 0;
    std::string funcNameBuf;
    funcNameBuf.reserve(128);
    for (ImportLibrary const& lib : peFile.getImportLibraries()) {
        std::string const& libName = lib.dllName;
        librariesFH.reduce(libName.c_str(), 1.0);

        for (ImportFunction const& func : lib.functions) {
            funcNameBuf.assign(libName);
            funcNameBuf.append(":");
            if (!func.name.empty()) {
                funcNameBuf.append(func.name);
            } else {
                funcNameBuf.append("ordinal");
                funcNameBuf.append(std::to_string(func.ordinal));
            }

            functionsFH.reduce(funcNameBuf.c_str(), 1.0);
            ++numFunctions;
        }
    }

    librariesFH.finalize();
    functionsFH.finalize();

    output[Offset::NUM_FUNCTIONS] = numFunctions;
    output[Offset::NUM_LIBRARIES] = numLibraries;
}

void ImportsInfo::reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) {
    // pass
}

void ImportsInfo::finalize(feature_t* output, PEFile const& peFile) {
    // pass
}

size_t ImportsInfo::getMaxDim() const {
    return DIM;
}
