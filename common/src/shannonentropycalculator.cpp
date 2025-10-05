#include "efe/common/shannonentropycalculator.h"
#include <cmath>

entropy_t calculateShannonEntropy(size_t sumTotalCounts, size_t const* const counts, size_t numCounts) {
    if (sumTotalCounts == 0) {
        return 0.0;
    }

    entropy_t entropy = 0.0;
    const entropy_t invTotal = 1.0 / sumTotalCounts;

    for (size_t i = 0; i < numCounts; ++i) {
        auto const x = counts[i];
        if (x == 0) continue; // skip zero counts
        entropy_t p_x = x * invTotal;
        entropy -= x * std::log2(p_x);
    }

    return entropy;
}
