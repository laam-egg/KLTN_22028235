#ifndef SHANNON_ENTROPY_CALCULATOR_INCLUDED
#define SHANNON_ENTROPY_CALCULATOR_INCLUDED

#include "efe/common/bytecounter.h"

using entropy_t = float;

entropy_t calculateShannonEntropy(size_t sumTotalCounts, size_t const* const counts, size_t numCounts);

#endif // SHANNON_ENTROPY_CALCULATOR_INCLUDED
