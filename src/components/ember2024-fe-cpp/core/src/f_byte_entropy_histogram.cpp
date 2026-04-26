#include "efe/core/f_byte_entropy_histogram.h"
#include "efe/common/shannonentropycalculator.h"
#include "efe/common/meth.h"

// #define MODULO_WINDOW_SIZE(x) ((x) & (WINDOW_SIZE - 1))

ByteEntropyHistogram::~ByteEntropyHistogram() = default;

char const* ByteEntropyHistogram::getFeatureName() {
    return "byteentropy";
}

char const* ByteEntropyHistogram::getName() const {
    return getFeatureName();
}

size_t ByteEntropyHistogram::getMaxDim() const {
    return DIM;
}

void ByteEntropyHistogram::reset(feature_t* output, PEFile const& peFile) {
    slidingByteWindow.reset();
    slidingByteWindow.setWindowSize(WINDOW_SIZE);
    slidingByteWindow.setStep(WINDOW_STEP);
    slidingByteWindow.setCallback(
        std::bind(
            &ByteEntropyHistogram::withCompleteWindow, this,
            std::placeholders::_1, std::placeholders::_2
        )
    );
}

void ByteEntropyHistogram::start(feature_t* output, PEFile const& peFile) {
    withCompleteWindowCalled = false;

    outputBuffer = output;

    std::memset(outputBuffer, 0, DIM * sizeof(feature_t));

    slidingByteWindow.start();
}

void ByteEntropyHistogram::reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) {
    slidingByteWindow.reduce(bufOffset, buf, bufSize);
}

void ByteEntropyHistogram::withCompleteWindow(uint8_t const* block, size_t blockSize) {
    this->withCompleteWindowCalled = true;

    writeEntropyBinCounts(outputBuffer, block, blockSize);
}

void ByteEntropyHistogram::finalize(feature_t* output, PEFile const& peFile) {
    slidingByteWindow.finalize();

    if (!withCompleteWindowCalled) {
        // No complete block formed => the only block is smaller than window size
        // (and is the last block, too).
        std::vector<uint8_t> const& theOnlyBlock = slidingByteWindow.getLastBlockAnyway();

        writeEntropyBinCounts(outputBuffer, theOnlyBlock.data(), theOnlyBlock.size());
    }

    feature_t sum = arraySum(0.0, outputBuffer, DIM);
    if (sum == 0) {
        // TODO: Temporarily do not normalize here.
    } else {
        for (size_t i = 0; i < DIM; ++i) {
            outputBuffer[i] /= sum;
        }
    }
}

/**
 * equivalent to _entropy_bin_counts() in Python implementation
 */
void ByteEntropyHistogram::writeEntropyBinCounts(feature_t* outputBuffer, uint8_t const* block, size_t blockSize) {
    byteCounter.reset();
    byteCounter.start();

    temp.resize(blockSize);
    for (size_t i = 0; i < blockSize; ++i) {
        temp[i] = block[i] >> 4;
    }
    byteCounter.reduce(temp.data(), temp.size());

    byteCounter.finalize();

    // Write to output vector

    feature_t* const output = outputBuffer;
    size_t const* const c = byteCounter.getByteCountsArray();

    entropy_t H = calculateShannonEntropy(byteCounter.getTotalNumBytes(), c, 16) * 2;
    int H_bin = (int)(H * 2);
    if (H_bin < 0) H_bin = 0;
    else if (H_bin > 15) H_bin = 15;

    feature_t* const vector = output + H_bin * 16;
    for (size_t i = 0; i < 16; ++i) {
        vector[i] += c[i];
    }
}
