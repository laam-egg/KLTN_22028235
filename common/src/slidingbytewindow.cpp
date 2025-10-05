#include "efe/common/slidingbytewindow.h"
#include "efe/common/logging.h"
#include "efe/common/meth.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>

SlidingByteWindow::SlidingByteWindow() {
    reset();
}

SlidingByteWindow::~SlidingByteWindow() {
    deallocateEverything();
}

void SlidingByteWindow::reset() {
    window = NULL;
    windowSize = 0;
    filledPartBeginPos = filledPartSize = 0;
    callback = NULL;
}

void SlidingByteWindow::deallocateEverything() {
    if (window != NULL) {
        delete[] window;
        window = NULL;
    }
}

void SlidingByteWindow::setWindowSize(size_t newWindowSize) {
    windowSize = newWindowSize;
}

void SlidingByteWindow::setStep(size_t newStep) {
    step = newStep;
}

void SlidingByteWindow::setCallback(WithCompleteWindowCallback newCallback) {
    callback = newCallback;
}

void SlidingByteWindow::start() {
    if (windowSize == 0) {
        LOG_FATAL_ERROR("window size is zero or not set");
    }
    if (step == 0) {
        LOG_FATAL_ERROR("step is zero or not set");
    }
    if (step > windowSize) {
        LOG_FATAL_ERROR("step > windowSize is not supported (step = %zu, windowSize = %zu)", step, windowSize);
    }
    if (!isPowerOf2(windowSize)) {
        LOG_FATAL_ERROR("windowSize is not a power of 2 - this breaks modulo computation when an overflow occurs, hence not allowed.");
    }
    if (callback == NULL) {
        LOG_FATAL_ERROR("callback not set");
    }

    window = new uint8_t[windowSize];
}


void SlidingByteWindow::reduce(size_t bufOffset, uint8_t const* buf, size_t bufSize) {
    // #define MODULO_WINDOW_SIZE(x) ((x) % windowSize)
    // WARNING: Only legit if windowSize is a power of 2 (which it is, in this case):
    #define MODULO_WINDOW_SIZE(x) ((x) & (windowSize - 1))

    size_t readPos = 0;

    while (readPos < bufSize) {
        size_t bytesToTake = std::min(bufSize - readPos, windowSize - filledPartSize);

        size_t writePos = MODULO_WINDOW_SIZE(filledPartBeginPos + filledPartSize);
        size_t bytesToWrite = std::min(windowSize - writePos, bytesToTake);
        if (bytesToWrite > 0) {
            std::memcpy(window + writePos, buf + readPos, bytesToWrite);
            readPos += bytesToWrite;
        }
        if (bytesToWrite < bytesToTake) {
            bytesToWrite = bytesToTake - bytesToWrite;
            writePos = 0;
            std::memcpy(window + writePos, buf + readPos, bytesToWrite);
            readPos += bytesToWrite;
        }

        filledPartSize += bytesToTake;
        if (filledPartSize == windowSize) {
            // callback(window, windowSize, filledPartSize, &filledPartBeginPos);
            callTheCallback();
            filledPartBeginPos = MODULO_WINDOW_SIZE(filledPartBeginPos + step);
            filledPartSize -= step;
        }
    }

    #undef MODULO_WINDOW_SIZE
}

void SlidingByteWindow::finalize() {
    if (filledPartSize > 0) {
        callTheCallback();
    }

    deallocateEverything();
}

void SlidingByteWindow::callTheCallback() noexcept {
    const size_t firstSegmentSize = std::min(windowSize - filledPartBeginPos, filledPartSize);
    const size_t secondSegmentSize = filledPartSize - firstSegmentSize;

    uint8_t* firstSegment = window + filledPartBeginPos;
    uint8_t* secondSegment = window;

    callback(firstSegment, firstSegmentSize, secondSegment, secondSegmentSize);
}
