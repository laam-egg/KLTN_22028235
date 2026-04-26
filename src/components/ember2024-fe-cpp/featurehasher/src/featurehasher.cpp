#include "efe/featurehasher.h"
#include "murmur3.h"
#include "efe/common/logging.h"
#include <cstring>
#include <cstdint>
#include <unordered_map>
#include <algorithm>

// #define LLL LOG_INFO
#define LLL(...)

struct CSRMatrix {
    int64_t n_rows;
    int64_t n_cols;

    std::vector<int64_t> indptr;   // length n_rows + 1
    std::vector<int32_t> indices;  // length nnz
    std::vector<double>  data;     // length nnz
};

void sum_duplicates_and_sort_csr(CSRMatrix &X);
std::vector<double> csr_to_dense(const CSRMatrix& X);

FeatureHasher::FeatureHasher() {
    reset();
}

void FeatureHasher::reset() {
    alternateSign = true;
    output = NULL;
    size = 0;
    seed = 0;
    csr = CSRResult();
    multipleSamples = false;
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

void FeatureHasher::setSeed(uint32_t newSeed) {
    seed = newSeed;
}

void FeatureHasher::setMultipleSamples(bool enable) {
    multipleSamples = enable;
}

void FeatureHasher::start() {
    if (output == NULL) {
        LOG_FATAL_ERROR("output is NULL or not set");
    }
    if (size == 0) {
        LOG_FATAL_ERROR("size is 0 or not set");
    }

    std::memset(output, 0, size * sizeof(output[0]));

    this->sz = 0;
    this->csr.values.resize(1); // it's 8192 in original Python code
    this->csr.indptr.push_back(0);
}

void FeatureHasher::reduce(char const* p1, feature_t p2) {
    LLL("HERE IT IS");

    if (p2 == 0.0) return;

    std::vector<int32_t>& indices = csr.indices;
    std::vector<int64_t>& indptr = csr.indptr;
    std::vector<double>& values = csr.values;

    LLL("HERE IT IS");
    size_t len = strlen(p1);
    int32_t h;
    MurmurHash3_x86_32(
        p1,                             // key, a pointer to the data you wish to hash;
        len,                            // len, the length in bytes;
        this->seed,                     // seed, an arbitrary seed number which you can use to tweak the hash;
        static_cast<void*>(&h)          // out, a pointer to a buffer big enough to hold the hash's output value.
    );
    LLL("HERE IT IS");

    // special case: abs(INT32_MIN) is UB in C++
    int32_t idx;
    if (h == INT32_MIN) {
        idx = (2147483647 - (this->size - 1)) % this->size;
    } else {
        idx = static_cast<int32_t>(std::abs(h) % this->size);
    }

    indices.push_back(idx);

    // Alternate sign as scikit-learn does
    if (alternateSign) {
        p2 *= (h >= 0 ? 1.0 : -1.0);
    }

    // Store value
    if (sz == static_cast<int64_t>(values.size())) {
        // enlarge
        values.resize(values.size() * 2);
    }

    LLL("BEFORE");
    values[sz++] = p2;
    LLL("AFTER");
}

void FeatureHasher::reduce(char const* stringValue) {
    LLL("BE 1");
    this->reduce(stringValue, 1);
    LLL("AF 1");
}

void FeatureHasher::finalizeSample() {
    LLL("BE 2");
    std::vector<int64_t>& indptr = csr.indptr;
    indptr.push_back(this->sz);
    LLL("AF 2");
}

void FeatureHasher::finalize() {
    LLL("BE 3");
    if (false == multipleSamples) {
        // Automatically finalize the only sample
        finalizeSample();
    }

    std::vector<int32_t>& indices = csr.indices;
    std::vector<int64_t>& indptr = csr.indptr;
    std::vector<double>& values = csr.values;

    // shrink to size
    values.resize(this->sz);

    // csr from _hashing_transform complete at this point.
    // https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/feature_extraction/_hash.py#L183

    int n_samples = static_cast<int>(indptr.size()) - 1;
    if (n_samples <= 0) {
        LOG_ERROR("Cannot vectorize empty sequence (n_samples = %d). Still continuing.", n_samples);
    } else {
        CSRMatrix X;
        X.n_rows = n_samples;
        X.n_cols = this->size; // n_features
        X.indptr = std::move(indptr);
        X.indices = std::move(indices);
        X.data = std::move(values);
        sum_duplicates_and_sort_csr(X);

        auto const arr = csr_to_dense(X);
        for (size_t i = 0; i < arr.size(); ++i) {
            output[i] = arr[i];
        }
    }

    buf.clear();
    LLL("AF 3");
}

/**
 * ChatGPT
 * converted from this:
 * https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/feature_extraction/_hash.py#L191C1-L198C17
 */
void sum_duplicates_and_sort_csr(CSRMatrix &X) {
    const int64_t n_rows = X.n_rows;

    std::vector<int32_t> new_indices;
    std::vector<double> new_data;
    new_indices.reserve(X.indices.size());
    new_data.reserve(X.data.size());

    std::vector<int64_t> new_indptr;
    new_indptr.reserve(n_rows + 1);
    new_indptr.push_back(0);

    for (int64_t r = 0; r < n_rows; r++) {
        int64_t start = X.indptr[r];
        int64_t end   = X.indptr[r+1];

        // -------- Step 1: accumulate duplicates into a map
        std::unordered_map<int32_t, double> rowmap;
        rowmap.reserve(end - start);

        for (int64_t i = start; i < end; i++) {
            rowmap[X.indices[i]] += X.data[i];
        }

        // -------- Step 2: move to a vector and sort by column index
        std::vector<std::pair<int32_t, double>> row_elems;
        row_elems.reserve(rowmap.size());

        for (auto &p : rowmap)
            row_elems.emplace_back(p.first, p.second);

        std::sort(row_elems.begin(), row_elems.end(),
                  [](auto &a, auto &b) { return a.first < b.first; });

        // -------- Step 3: append to new CSR storage
        for (auto &p : row_elems) {
            new_indices.push_back(p.first);
            new_data.push_back(p.second);
        }

        new_indptr.push_back(new_indices.size());
    }

    // overwrite with cleaned data
    X.indptr  = std::move(new_indptr);
    X.indices = std::move(new_indices);
    X.data    = std::move(new_data);
}

/**
 * ChatGPT
 * convert .to_array() of CSRMatrix, from Python to C++
 */
std::vector<double> csr_to_dense(const CSRMatrix& X) {
    std::vector<double> out(X.n_rows * X.n_cols, 0.0);

    for (int64_t r = 0; r < X.n_rows; ++r) {
        int64_t start = X.indptr[r];
        int64_t end   = X.indptr[r+1];

        for (int64_t k = start; k < end; ++k) {
            int32_t c = X.indices[k];
            out[r * X.n_cols + c] = X.data[k]; // row-major
        }
    }

    return out;
}
