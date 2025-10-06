#ifndef FEATURE_TYPE_STRING_EXTRACTOR_INCLUDED
#define FEATURE_TYPE_STRING_EXTRACTOR_INCLUDED

#include "efe/core/f_base.h"
#include "efe/common/bytecounter.h"
#include "efe/common/shannonentropycalculator.h"
#include <regex>

class StringExtractor final : public FeatureType {
private:
    static std::regex const INTERESTING_STRING_REGEXES[]; // TODO: convert to pattern "static function has static const variable and returns it" so that threads don't race to initialize these regexes
    static std::regex const READABLE_STRING_REGEX;

    size_t NUM_INTERESTING_STRING_REGEXES;
    double sumReadableStringLengths;
    size_t numReadableStrings;
    ByteCounter readableByteCounter;
    ByteCounter interestingStringCounter;

public:
    StringExtractor();

    virtual char const* getName() const override;

    virtual void reset(feature_t* output, PEFile const& peFile) override;

    virtual void start(feature_t* output, PEFile const& peFile) override;

    virtual void reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) override;

    virtual void finalize(feature_t* output, PEFile const& peFile) override;

    virtual size_t getMaxDim() const override;
};

#endif // FEATURE_TYPE_STRING_EXTRACTOR_INCLUDED
