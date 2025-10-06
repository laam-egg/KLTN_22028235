#ifndef EFE_FEATURE_HASHER_INCLUDED
#define EFE_FEATURE_HASHER_INCLUDED

#include <cstddef>
#include <string>

using feature_t = float;

typedef size_t (*StringHashFunction)(char const*);

class FeatureHasher {
private:
    bool alternateSign;
    feature_t* output;
    size_t size;
    StringHashFunction stringHashFunction;
    std::string buf;

public:
    FeatureHasher();

    void reset();

    void setOutput(feature_t* newOutput);

    void setSize(size_t newSize);

    void setAlternateSign(bool enable);

    void setStringHashFunction(StringHashFunction newStringHashFunction);

    void start();

    void reduce(char const* p1, feature_t p2);

    void finalize();
};

#endif // EFE_FEATURE_HASHER_INCLUDED
