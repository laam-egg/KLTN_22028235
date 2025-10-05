#include "efe/core/f_byte_entropy_histogram.h"
#include "efe/common/shannonentropycalculator.h"

// #define MODULO_WINDOW_SIZE(x) ((x) & (WINDOW_SIZE - 1))

char const* ByteEntropyHistogram::getName() const {
    return "byteentropy";
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
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4
        )
    );
}

void ByteEntropyHistogram::start(feature_t* output, PEFile const& peFile) {
    isFirstWindow = true;
    outputBuffer = output;

    std::memset(outputBuffer, 0, DIM * sizeof(feature_t));

    slidingByteWindow.start();
}

void ByteEntropyHistogram::reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) {
    slidingByteWindow.reduce(bufOffset, buf, bufSize);
}

void ByteEntropyHistogram::withCompleteWindow(uint8_t* firstSegment, size_t firstSegmentSize, uint8_t* secondSegment, size_t secondSegmentSize) {
    byteCounter.reset();
    byteCounter.start();

    size_t normalizationBeginPos = isFirstWindow ? 0 : WINDOW_STEP;
    isFirstWindow = false;

    if (firstSegmentSize > 0) {
        for (size_t i = normalizationBeginPos; i < firstSegmentSize; ++i) {
            firstSegment[i] >>= 4;
        }
        byteCounter.reduce(firstSegment, firstSegmentSize);
    }

    if (secondSegmentSize > 0) {
        // for (size_t i = std::max(normalizationBeginPos, firstSegmentSize); i < firstSegmentSize + secondSegmentSize; ++i) {
        //     secondSegment[i - firstSegmentSize] >>= 4;
        // }
        size_t i;
        if (normalizationBeginPos > firstSegmentSize) {
            i = normalizationBeginPos - firstSegmentSize;
        } else {
            i = 0;
        }
        for (; i < secondSegmentSize; ++i) {
            secondSegment[i] >>= 4;
        }
        byteCounter.reduce(secondSegment, secondSegmentSize);
    }

    byteCounter.finalize();

    feature_t* const output = outputBuffer;
    size_t const* const c = byteCounter.getByteCountsArray();

    entropy_t H = calculateShannonEntropy(byteCounter.getTotalNumBytes(), c, 16);
    int H_bin = (int)(H * 2);
    if (H_bin < 0) H_bin = 0;
    else if (H_bin > 15) H_bin = 15;

    feature_t* const vector = output + H_bin * 16;
    for (size_t i = 0; i < 16; ++i) {
        vector[i] += c[i];
    }
}

void ByteEntropyHistogram::finalize(feature_t* output, PEFile const& peFile) {
    slidingByteWindow.finalize();
}
