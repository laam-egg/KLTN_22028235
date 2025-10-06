#include "efe/core/f_pe_format_warnings.h"

#define DIM 88

char const* PEFormatWarnings::getName() const {
    return "pefilewarnings";
}

void PEFormatWarnings::reset(feature_t* output, PEFile const& peFile) {
    // pass
}

void PEFormatWarnings::start(feature_t* output, PEFile const& peFile) {
    // TODO: Implement this later
    std::memset(output, 0, DIM * sizeof(output[0]));
}

void PEFormatWarnings::reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) {
    // pass
}

void PEFormatWarnings::finalize(feature_t* output, PEFile const& peFile) {
    // pass
}

size_t PEFormatWarnings::getMaxDim() const {
    return DIM;
}
