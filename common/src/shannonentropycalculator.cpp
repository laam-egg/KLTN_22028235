#include "efe/common/shannonentropycalculator.h"
#include <cmath>

entropy_t calculateShannonEntropy(size_t sumTotalCounts, size_t const* const counts, size_t numCounts) {
    if (sumTotalCounts == 0) {
        return 0.0;
    }

    entropy_t entropy = 0.0;
    
    #ifdef OPTIMIZATION_MISMATCHING_ORIGINAL
    const entropy_t invTotal = 1.0 / sumTotalCounts;
    #endif

    for (size_t i = 0; i < numCounts; ++i) {
        auto const x = counts[i];
        if (x == 0) continue; // skip zero counts
        entropy_t p_x = (
            #ifdef OPTIMIZATION_MISMATCHING_ORIGINAL
            x * invTotal
            #else
            float(x) / sumTotalCounts
            #endif
        );
        entropy -= p_x * std::log2(p_x);
    }

    return entropy;
}
