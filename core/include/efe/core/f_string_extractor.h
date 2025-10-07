#ifndef FEATURE_TYPE_STRING_EXTRACTOR_INCLUDED
#define FEATURE_TYPE_STRING_EXTRACTOR_INCLUDED

#include "efe/core/f_base.h"
#include "efe/common/bytecounter.h"
#include "efe/common/shannonentropycalculator.h"
#include <re2/re2.h>

class StringExtractor final : public FeatureType {
private:
    static RE2 const* getInterestingStringRegexes(size_t& count);
    static RE2 const& getReadableStringRegex();

    RE2 const* interestingStringRegexes;
    RE2 const& readableStringRegex;
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
