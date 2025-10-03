#include "efe/common/bytecounter.h"
#include <cstring>

ByteCounter::ByteCounter() {}

void ByteCounter::reset() {
    totalNumBytes = 0;
    std::memset(byteCounts, 0, sizeof(byteCounts));
}

void ByteCounter::start() {}

void ByteCounter::reduce(char const* buf, size_t bufSize) {
    for (size_t i = 0; i < bufSize; ++i) {
        ++byteCounts[buf[i]];
    }
    totalNumBytes += bufSize;
}

void ByteCounter::finalize() {}
