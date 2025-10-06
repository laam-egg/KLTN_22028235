#ifndef FEATURE_TYPE_DATA_DIRECTORIES_INCLUDED
#define FEATURE_TYPE_DATA_DIRECTORIES_INCLUDED

#include "efe/core/f_base.h"

class DataDirectories final : public FeatureType {
public:
    virtual char const* getName() const override;

    virtual void reset(feature_t* output, PEFile const& peFile) override;

    virtual void start(feature_t* output, PEFile const& peFile) override;

    virtual void reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) override;

    virtual void finalize(feature_t* output, PEFile const& peFile) override;

    virtual size_t getMaxDim() const override;
};

#endif // FEATURE_TYPE_DATA_DIRECTORIES_INCLUDED
