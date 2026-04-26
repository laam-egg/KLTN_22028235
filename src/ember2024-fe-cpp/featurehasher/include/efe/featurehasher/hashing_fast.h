/**
 * Translated by ChatGPT from _hashing_fast.pyx
 * https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/feature_extraction/_hashing_fast.pyx
 * 
 * For reference only. Code in this file
 * won't be run anywhere (even if it's
 * added in CMakeLists.txt)
 */


#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <cmath>
#include <cstring>
#include "murmur3.h"

struct CSRResult {
    std::vector<int32_t> indices;
    std::vector<int64_t> indptr;
    std::vector<double> values;
};

// raw_X: vector of samples, each sample = vector of (feature, value)
// feature = string
// value can be string or numeric -> here, assume string = categorical, double = numeric
// Adapt as needed.
struct FeatureValue {
    std::string feature;
    bool is_string_value;
    std::string string_value;   // used if is_string_value == true
    double numeric_value;       // used otherwise
};

typedef size_t (*StringHashFunction)(char const*);

CSRResult transform(
    StringHashFunction hashString,
    const std::vector<std::vector<FeatureValue>>& raw_X,
    int64_t n_features,
    bool alternate_sign = true,
    uint32_t seed = 0
) {
    CSRResult out;

    std::vector<int32_t>& indices = out.indices;
    std::vector<int64_t>& indptr = out.indptr;
    std::vector<double>& values = out.values;

    indptr.push_back(0);

    size_t capacity = 8192;
    values.resize(capacity);
    int64_t size = 0;

    for (const auto& sample : raw_X) {
        for (const auto& fv : sample) {

            std::string f = fv.feature;
            double value;

            // If v is string → f = f + "=" + v, value = 1
            if (fv.is_string_value) {
                f = f + "=" + fv.string_value;
                value = 1.0;
            } else {
                value = fv.numeric_value;
            }

            if (value == 0.0)
                continue;

            // f must be UTF-8 bytes; assume already valid std::string
            const uint8_t* data = reinterpret_cast<const uint8_t*>(f.data());
            size_t len = f.size();

            // int32_t h = MurmurHash3_x86_32(data, len, seed);
            int32_t h;
            MurmurHash3_x86_32(
                data,                           // key, a pointer to the data you wish to hash;
                len,                            // len, the length in bytes;
                seed,                           // seed, an arbitrary seed number which you can use to tweak the hash;
                static_cast<void*>(&h)          // out, a pointer to a buffer big enough to hold the hash's output value.
            );

            // special case: abs(INT32_MIN) is UB in C++
            int32_t idx;
            if (h == INT32_MIN) {
                idx = (2147483647 - (n_features - 1)) % n_features;
            } else {
                idx = static_cast<int32_t>(std::abs(h) % n_features);
            }

            indices.push_back(idx);

            // Alternate sign as scikit-learn does
            if (alternate_sign) {
                value *= (h >= 0 ? 1.0 : -1.0);
            }

            // Store value
            if (size == static_cast<int64_t>(values.size())) {
                // enlarge
                values.resize(values.size() * 2);
            }

            values[size++] = value;
        }

        indptr.push_back(size);
    }

    // shrink to size
    values.resize(size);

    return out;
}
