#include "efe/core/f_base.h"
#include "efe/common/bytecounter.h"
#include "efe/common/bytecatcher.h"

class GeneralFileInfo final : public FeatureType {
private:
    ByteCounter byteCounter;
    ByteCatcher byteAtOffsetCatcher;
    static constexpr size_t const NUM_FIRST_BYTES_TO_TAKE = 3;

public:
    virtual char const* getName() const override {
        return "general";
    }

    virtual void reset(feature_t* output, PEFile const& peFile) override {
        byteCounter.reset();

        byteAtOffsetCatcher.reset();
        byteAtOffsetCatcher.wantByteAtOffset(0);
        byteAtOffsetCatcher.wantByteAtOffset(1);
        byteAtOffsetCatcher.wantByteAtOffset(2);
        byteAtOffsetCatcher.wantByteAtOffset(3);
    }

    virtual void start(feature_t* output, PEFile const& peFile) override {
        byteCounter.start();

        byteAtOffsetCatcher.start();
    }

    virtual bool reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, char const* buf, size_t bufSize) override {
        byteCounter.reduce(buf, bufSize);

        byteAtOffsetCatcher.reduce(bufOffset, buf, bufSize);
    }

    virtual void finalize(feature_t* output, PEFile const& peFile) override {
        byteCounter.finalize();
        byteAtOffsetCatcher.finalize();

        feature_t const f_size = static_cast<feature_t>(byteCounter.getTotalNumBytes());
        feature_t const f_entropy = 0.0; // TODO
        feature_t const f_isPE = peFile.isPEFile() ? 1.0 : 0.0;

        output[0] = f_size;
        output[1] = f_entropy;
        output[2] = f_isPE;

        feature_t* f_startBytes = output + 3;
        auto const& offsetToByteMap = byteAtOffsetCatcher.getOffsetToByteMap();
        for (size_t i = 0; i < NUM_FIRST_BYTES_TO_TAKE; ++i) {
            auto const it = offsetToByteMap.find(i);
            if (it != offsetToByteMap.end()) {
                f_startBytes[i] = it->second;
            } else {
                f_startBytes[i] = 0.0;
            }
        }
    }

    virtual size_t getMaxDim() const override {
        return 3 + NUM_FIRST_BYTES_TO_TAKE;
    }
};
