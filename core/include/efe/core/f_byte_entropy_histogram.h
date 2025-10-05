#ifndef FEATURE_BYTE_ENTROPY_HISTOGRAM_INCLUDED
#define FEATURE_BYTE_ENTROPY_HISTOGRAM_INCLUDED

#include "efe/core/f_base.h"
#include "efe/common/bytecounter.h"
#include "efe/common/slidingbytewindow.h"

class ByteEntropyHistogram final : public FeatureType {
private:
    static constexpr size_t const DIM = 256;

    static constexpr size_t const WINDOW_SIZE = 2048;

    static constexpr size_t const WINDOW_STEP = 1024;

    SlidingByteWindow slidingByteWindow;

    ByteCounter byteCounter;

    bool isFirstWindow;

    void withCompleteWindow(uint8_t* firstSegment, size_t firstSegmentSize, uint8_t* secondSegment, size_t secondSegmentSize);

    feature_t* outputBuffer;

public:
    virtual char const* getName() const override;

    virtual size_t getMaxDim() const override;

    virtual void reset(feature_t* output, PEFile const& peFile) override;

    virtual void start(feature_t* output, PEFile const& peFile) override;

    virtual void reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) override;

    virtual void finalize(feature_t* output, PEFile const& peFile) override;
};

#endif // FEATURE_BYTE_ENTROPY_HISTOGRAM_INCLUDED
