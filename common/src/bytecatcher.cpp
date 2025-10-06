#include "efe/common/bytecatcher.h"

ByteCatcher::ByteCatcher() {}

void ByteCatcher::reset() {
    offsetToByteMap.clear();
}

void ByteCatcher::wantByteAtOffset(size_t offset) {
    offsetToByteMap.insert({ offset, 0 });
}

void ByteCatcher::start() {
    size_t maxKey = 0;

    for (auto const& [key, value] : offsetToByteMap) {
        if (key > maxKey) maxKey = key;
    }

    maxOffsetToLookFor = maxKey;
}

void ByteCatcher::reduce(size_t bufOffset, uint8_t const* buf, size_t bufSize) {
    if (offsetToByteMap.empty() || bufOffset > maxOffsetToLookFor) {
        return;
    }

    size_t upperOffsetLimitExclusive = bufOffset + bufSize;

    for (auto it = offsetToByteMap.begin(); it != offsetToByteMap.end(); ++it) {
        size_t absoluteOffset = it->first;
        if (absoluteOffset >= bufOffset && absoluteOffset < upperOffsetLimitExclusive) {
            size_t relativeOffset = absoluteOffset - bufOffset;
            it->second = buf[relativeOffset];
        }
    }
}

void ByteCatcher::finalize() {}
