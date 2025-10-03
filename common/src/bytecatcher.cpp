#include "efe/common/bytecatcher.h"
#include <limits>

ByteCatcher::ByteCatcher() {}

void ByteCatcher::reset() {
    offsetToByteMap.clear();
}

void ByteCatcher::wantByteAtOffset(size_t offset) {
    offsetToByteMap.insert({ offset, 0 });
}

void ByteCatcher::start() {
    // size_t minKey = std::numeric_limits<size_t>::max();
    size_t maxKey = std::numeric_limits<size_t>::min();

    for (auto const& [key, value] : offsetToByteMap) {
        // if (key < minKey) minKey = key;
        if (key > maxKey) maxKey = key;
    }

    maxOffsetToLookFor = maxKey;
    // minOffsetToLookFor = minKey;
}

void ByteCatcher::reduce(size_t bufOffset, char const* buf, size_t bufSize) {
    if (bufOffset > maxOffsetToLookFor) {
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
