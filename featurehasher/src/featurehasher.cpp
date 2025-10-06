#include "efe/featurehasher.h"
#include "murmur3.h"
#include "efe/common/logging.h"
#include <cstring>
#include <cstdint>

static size_t StringHasher_MurmurHash3_x86_32(char const* str) {
    size_t strBufferSizeInBytes = strlen(str);
    uint32_t output;
    MurmurHash3_x86_32(
        static_cast<void const*>(str),  // key, a pointer to the data you wish to hash;
        strBufferSizeInBytes,           // len, the length in bytes;
        0,                              // seed, an arbitrary seed number which you can use to tweak the hash;
        static_cast<void*>(&output)     // out, a pointer to a buffer big enough to hold the hash's output value.
    );
    return static_cast<size_t>(output);
}

FeatureHasher::FeatureHasher() {
    reset();
}

void FeatureHasher::reset() {
    alternateSign = false;
    output = NULL;
    size = 0;
    stringHashFunction = StringHasher_MurmurHash3_x86_32;
}

void FeatureHasher::setOutput(feature_t* newOutput) {
    output = newOutput;
}

void FeatureHasher::setSize(size_t newSize) {
    size = newSize;
}

void FeatureHasher::setAlternateSign(bool enable) {
    alternateSign = enable;
}

void FeatureHasher::setStringHashFunction(StringHashFunction newStringHashFunction) {
    stringHashFunction = newStringHashFunction;
}

void FeatureHasher::start() {
    if (output == NULL) {
        LOG_FATAL_ERROR("output is NULL or not set");
    }
    if (size == 0) {
        LOG_FATAL_ERROR("size is 0 or not set");
    }
    if (stringHashFunction == NULL) {
        LOG_FATAL_ERROR("stringHashFunction is NULL");
    }

    std::memset(output, 0, size * sizeof(output[0]));
}

void FeatureHasher::reduce(char const* p1, feature_t p2) {
    size_t h = stringHashFunction(p1);
    size_t index = h % size;

    feature_t sign = 1.0;
    if (alternateSign) {
        // Use a second hash to decide sign (odd/even)
        buf.clear();
        char const signSuffix[] = "_sign";
        buf.reserve(strlen(p1) + sizeof(signSuffix) / sizeof(signSuffix[0]));
        buf.append(p1);
        buf.append("_sign");
        size_t signHash = stringHashFunction(buf.c_str());

        if (signHash & 1) {
            sign = -1.0;
        }
    }

    output[index] += sign * p2;
}

void FeatureHasher::finalize() {
    buf.clear();
}
