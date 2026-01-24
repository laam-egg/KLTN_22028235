#include "efe/core/f_authenticode_signature.h"

#define DIM 8

extern "C" typedef struct _AuthenticodeSignatureFeatures {
    feature_t numSignatures;
    feature_t selfSigned;
    feature_t emptyProgramName;
    feature_t noCounterSigner;
    feature_t parseError;
    feature_t chainMaxDepth;
    feature_t latestSigningTime;
    feature_t signingTimeDiff;
} AuthenticodeSignatureFeatures;

static_assert(sizeof(AuthenticodeSignatureFeatures) == DIM * sizeof(feature_t), "Dimension mismatch");




AuthenticodeSignature::~AuthenticodeSignature() = default;

char const* AuthenticodeSignature::getFeatureName() {
    return "authenticode";
}

char const* AuthenticodeSignature::getName() const {
    return getFeatureName();
}

void AuthenticodeSignature::reset(feature_t* output, PEFile const& peFile) {
    // pass
}

void AuthenticodeSignature::start(feature_t* output, PEFile const& peFile) {
    std::memset(output, 0, DIM * sizeof(output[0]));

    if (!peFile.isPEFile()) {
        return;
    }

    AuthenticodeSignatureFeatures* features =
        reinterpret_cast<AuthenticodeSignatureFeatures*>(output);
    
    PEAuthenticodeSignatureInfo const authInfo = peFile.getAuthenticodeSignatureInfo();
    features->numSignatures = static_cast<feature_t>(authInfo.numSignatures);
    features->selfSigned = static_cast<feature_t>(authInfo.numSelfSignedCerts > 0 ? 1 : 0);
    features->emptyProgramName = static_cast<feature_t>(authInfo.emptyProgramName ? 1.0f : 0.0f);
    features->noCounterSigner = static_cast<feature_t>(authInfo.noCounterSigner ? 1.0f : 0.0f);
    features->parseError = static_cast<feature_t>(authInfo.parseErrors > 0 ? 1 : 0);
    features->chainMaxDepth = static_cast<feature_t>(authInfo.chainMaxDepth);
    features->latestSigningTime = static_cast<feature_t>(authInfo.latestSigningTime);
    features->signingTimeDiff = static_cast<feature_t>(authInfo.signingTimeDiff);
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
