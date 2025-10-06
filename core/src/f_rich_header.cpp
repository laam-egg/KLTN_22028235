#include "efe/core/f_rich_header.h"

#define DIM 33

#define PAIRS_FH_BUCKET_SIZE 32

class Offset {
public:
    #define ADD(NAME, VALUE) static constexpr size_t const NAME = VALUE;

    ADD(NUM_DWORD_PAIRS, 0)
    ADD(PAIRS_HASHED, NUM_DWORD_PAIRS + 1)

    #undef ADD

    static_assert(PAIRS_HASHED + PAIRS_FH_BUCKET_SIZE == DIM);
};

char const* RichHeaderFeatureType::getName() const {
    return "richheader";
}

void RichHeaderFeatureType::reset(feature_t* output, PEFile const& peFile) {
    // pass
}

void RichHeaderFeatureType::start(feature_t* output, PEFile const& peFile) {
    std::memset(output, 0, DIM * sizeof(output[0]));

    if (!peFile.isPEFile() || !peFile.hasRichHeader()) {
        return;
    }

    uint8_t const* rhBuf = NULL;
    size_t rhSize = 0;
    // Alignment: std::vector in peFile.getRichHeaderBytes is
    // constructed fresh, so no worry, we could cast to uint32_t
    // later down below
    peFile.getRichHeaderBytes(&rhBuf, &rhSize);
    if (rhBuf == NULL || rhSize == 0) {
        return;
    }

    size_t rhNumDwords = rhSize / sizeof(uint32_t);
    if (rhNumDwords < 2) {
        return;
    }
    
    FeatureHasher fh;
    fh.reset();
    fh.setOutput(output + Offset::PAIRS_HASHED);
    fh.setSize(PAIRS_FH_BUCKET_SIZE);
    fh.start();

    size_t numDwordPairs = rhNumDwords / 2;
    uint32_t const* const rhDwordBuf = reinterpret_cast<uint32_t const*>(rhBuf);
    output[Offset::NUM_DWORD_PAIRS] = static_cast<feature_t>(numDwordPairs);
    std::string buf;
    buf.reserve(8);
    for (size_t pairIdx = 0; pairIdx < numDwordPairs; ++pairIdx) {
        size_t i = pairIdx * 2;
        size_t j = i + 1;
        buf = std::to_string(rhDwordBuf[i]);
        fh.reduce(buf.c_str(), rhDwordBuf[j]);
    }

    fh.finalize();
}

void RichHeaderFeatureType::reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) {
    // pass
}

void RichHeaderFeatureType::finalize(feature_t* output, PEFile const& peFile) {
    // pass
}

size_t RichHeaderFeatureType::getMaxDim() const {
    return DIM;
}
