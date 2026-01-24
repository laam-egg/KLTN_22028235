#ifndef PEFILE_AUTHENTICODE_SIGNATURE_INCLUDED
#define PEFILE_AUTHENTICODE_SIGNATURE_INCLUDED

#include <cstdint>
#include <LIEF/PE.hpp>

struct PEAuthenticodeSignatureInfo {
    size_t numSignatures = 0;
    size_t numSelfSignedCerts = 0;
    bool emptyProgramName = false;
    bool noCounterSigner = false;
    size_t parseErrors = 0;
    size_t chainMaxDepth = 0;
    uint64_t latestSigningTime = 0;
    int64_t signingTimeDiff = 0;

    static PEAuthenticodeSignatureInfo extractFromPEFile(
        LIEF::PE::Binary const& pe
    );
};

#endif // PEFILE_AUTHENTICODE_SIGNATURE_INCLUDED
