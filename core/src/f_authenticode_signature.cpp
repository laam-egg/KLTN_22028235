#include "efe/core/f_authenticode_signature.h"

#define DIM 8

char const* AuthenticodeSignature::getName() const {
    return "authenticode";
}

void AuthenticodeSignature::reset(feature_t* output, PEFile const& peFile) {
    // pass
}

void AuthenticodeSignature::start(feature_t* output, PEFile const& peFile) {
    // TODO: Implement this later
    std::memset(output, 0, DIM * sizeof(output[0]));
}

void AuthenticodeSignature::reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) {
    // pass
}

void AuthenticodeSignature::finalize(feature_t* output, PEFile const& peFile) {
    // pass
}

size_t AuthenticodeSignature::getMaxDim() const {
    return DIM;
}
