#ifndef EFE_ALL_FEATURE_TYPES_INCLUDED
#define EFE_ALL_FEATURE_TYPES_INCLUDED

#include "efe/core/f_base.h"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>

class AllFeatureTypes final : public FeatureType {
private:
    std::vector<std::unique_ptr<FeatureType>> featureTypes;
    size_t dim;

public:
    AllFeatureTypes();

    virtual char const* getName() const override;

    virtual void reset(feature_t* output, PEFile const& peFile) override;

    virtual void start(feature_t* output, PEFile const& peFile) override;

    virtual void reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) override;

    virtual void finalize(feature_t* output, PEFile const& peFile) override;

    virtual size_t getMaxDim() const override;
};

#endif // EFE_ALL_FEATURE_TYPES_INCLUDED
