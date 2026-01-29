#ifndef EFE_FEATURE_HASHER_INCLUDED
#define EFE_FEATURE_HASHER_INCLUDED

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using feature_t = float;

struct CSRResult {
    std::vector<int32_t> indices;
    std::vector<int64_t> indptr;
    std::vector<double> values;
};

class FeatureHasher {
private:
    bool alternateSign;
    feature_t* output;
    size_t size;
    std::string buf;
    uint32_t seed;
    CSRResult csr;
    bool multipleSamples;
    int64_t sz;

public:
    FeatureHasher();

    void reset();

    void setOutput(feature_t* newOutput);

    void setSize(size_t newSize);

    void setAlternateSign(bool enable);

    void setSeed(uint32_t newSeed);

    void setMultipleSamples(bool enable);

    void start();

    void reduce(char const* p1, feature_t p2);

    void reduce(char const* stringValue);

    void finalizeSample();

    void finalize();
};

#endif // EFE_FEATURE_HASHER_INCLUDED
