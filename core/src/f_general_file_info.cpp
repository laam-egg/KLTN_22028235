#include "efe/core/f_general_file_info.h"

size_t const GeneralFileInfo::NUM_FIRST_BYTES_TO_TAKE = 3;

char const* GeneralFileInfo::getName() const {
    return "general,histogram";
}

void GeneralFileInfo::reset(feature_t* output, PEFile const& peFile) {
    byteCounter.reset();

    byteAtOffsetCatcher.reset();
    byteAtOffsetCatcher.wantByteAtOffset(0);
    byteAtOffsetCatcher.wantByteAtOffset(1);
    byteAtOffsetCatcher.wantByteAtOffset(2);
    byteAtOffsetCatcher.wantByteAtOffset(3);
}

void GeneralFileInfo::start(feature_t* output, PEFile const& peFile) {
    byteCounter.start();

    byteAtOffsetCatcher.start();
}

void GeneralFileInfo::reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) {
    byteCounter.reduce(buf, bufSize);

    byteAtOffsetCatcher.reduce(bufOffset, buf, bufSize);
}

void GeneralFileInfo::finalize(feature_t* output, PEFile const& peFile) {
    byteCounter.finalize();
    byteAtOffsetCatcher.finalize();

    size_t const* const byteCountArray = byteCounter.getByteCountsArray();

    feature_t const f_size = static_cast<feature_t>(byteCounter.getTotalNumBytes());
    feature_t const f_entropy = calculateShannonEntropy(f_size, byteCountArray, 256);
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

    feature_t* f_byteHistogram = f_startBytes + NUM_FIRST_BYTES_TO_TAKE;
    for (size_t i = 0; i < 256; ++i) {
        f_byteHistogram[i] = byteCountArray[i] / f_size; // type: feature_t
    }
}

size_t GeneralFileInfo::getMaxDim() const {
    return 3 + NUM_FIRST_BYTES_TO_TAKE + 256;
}
