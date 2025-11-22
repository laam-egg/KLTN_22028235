#include "efe/common/slidingbytewindow.h"
#include "efe/common/logging.h"
#include "efe/common/meth.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>

SlidingByteWindow::SlidingByteWindow() {
    reset();
}

SlidingByteWindow::~SlidingByteWindow() = default;

void SlidingByteWindow::reset() {
    block.clear();
    windowSize = 0;
    step = 0;
    callback = nullptr;
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
    if (callback == NULL) {
        LOG_FATAL_ERROR("callback not set");
    }
    if (step > windowSize) {
        LOG_FATAL_ERROR("step is greater than window size - this is not supported");
    }

    this->block.reserve(this->windowSize);
    this->block.clear();
}


void SlidingByteWindow::reduce(size_t bufOffset, uint8_t const* buf, size_t bufSize) {
    uint8_t const* endBuf = buf + bufSize;

    while (buf < endBuf) {
        size_t spaceInBlock = windowSize - this->block.size();
        if (spaceInBlock > 0) {
            size_t bytesToCopy = std::min(spaceInBlock, static_cast<size_t>(endBuf - buf));
            this->block.insert(this->block.end(), buf, buf + bytesToCopy);
            buf += bytesToCopy;
        } else {
            // Block is full, call the callback
            callTheCallback();
            // Slide the window by 'step' bytes
            this->block.erase(this->block.begin(), this->block.begin() + step);
        }
    }
}

void SlidingByteWindow::finalize() {
    // If there's any remaining data in the block that forms a complete window, process it
    if (this->block.size() == this->windowSize) {
        callTheCallback();
    }
}

void SlidingByteWindow::callTheCallback() noexcept {
    callback(this->block.data(), this->block.size());
}

std::vector<uint8_t> const& SlidingByteWindow::getLastBlockAnyway() const {
    return this->block;
}
