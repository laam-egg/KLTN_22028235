#ifndef FEATURE_BASE_INCLUDED
#define FEATURE_BASE_INCLUDED

#include "efe/common/pefile.h"

using feature_t = float;

/**
 * Base class from which each feature type may inherit
 */
class FeatureType {
public:
    virtual char const* getName() const = 0;

    virtual size_t getMaxDim() const = 0;

    virtual void reset(feature_t* output, PEFile const& peFile) = 0;

    virtual void start(feature_t* output, PEFile const& peFile) = 0;

    virtual bool reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, char const* buf, size_t bufSize) = 0;

    virtual void finalize(feature_t* output, PEFile const& peFile) = 0;
};

#endif // FEATURE_BASE_INCLUDED
