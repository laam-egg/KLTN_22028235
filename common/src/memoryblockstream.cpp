#include "efe/common/memoryblockstream.h"
#include <algorithm>
#include <cstring>

void MemoryBlockStream::setup(uint8_t const* buf, size_t bufSize) {
    m_buf = buf;
    m_bufSize = bufSize;
}

bool MemoryBlockStream::doStartReading() {
    if (m_buf == NULL) {
        return false;
    }
    return true;
}

void MemoryBlockStream::doStopReading() {
    m_buf = NULL;
    m_bufSize = 0;
}

size_t MemoryBlockStream::doReadNext(uint8_t* buf) {
    size_t blockSize = getBlockSize();
    size_t offset = getOffset();
    size_t numBytesLeft = m_bufSize - offset;
    size_t numBytesRead = std::min(numBytesLeft, blockSize);

    memcpy(buf, m_buf + offset, numBytesRead);
    return numBytesRead;
}
