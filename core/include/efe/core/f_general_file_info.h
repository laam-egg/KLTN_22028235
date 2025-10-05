#ifndef FEATURE_GENERAL_FILE_INFO_INCLUDED
#define FEATURE_GENERAL_FILE_INFO_INCLUDED

#include "efe/core/f_base.h"
#include "efe/common/bytecounter.h"
#include "efe/common/bytecatcher.h"
#include "efe/common/shannonentropycalculator.h"

/**
 * Corresponds to the following original feature types:
 * 
 *     GeneralFileInfo
 *     ByteHistogram
 * 
 */
class GeneralFileInfo final : public FeatureType {
private:
    ByteCounter byteCounter;
    ByteCatcher byteAtOffsetCatcher;
    static size_t const NUM_FIRST_BYTES_TO_TAKE;

public:
    virtual char const* getName() const override;

    virtual void reset(feature_t* output, PEFile const& peFile) override;

    virtual void start(feature_t* output, PEFile const& peFile) override;

    virtual void reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) override;

    virtual void finalize(feature_t* output, PEFile const& peFile) override;

    virtual size_t getMaxDim() const override;
};

#endif // FEATURE_GENERAL_FILE_INFO_INCLUDED
