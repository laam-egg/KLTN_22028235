#ifndef EFE_CORE_INCLUDED
#define EFE_CORE_INCLUDED

#include "efe/core/f_base.h"
#include "efe/core/all_feature_types.h"
#include <cstdint>
#include <vector>
#include <memory>

class EMBER2024FeatureExtractor {
private:
    std::unique_ptr<AllFeatureTypes> allFeatures;
    size_t dim;
    std::unique_ptr<uint8_t[]> buf;
    std::unique_ptr<feature_t[]> output;

public:
    EMBER2024FeatureExtractor();

    feature_t const* run(uint8_t const* fileContent, size_t fileSize);

    inline constexpr size_t getDim() const { return dim; }
};

#endif // EFE_CORE_INCLUDED
