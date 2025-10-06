#include "efe/common/blockstream.h"

void BlockStream::setBlockSize(size_t blockSize) {
    m_blockSize = blockSize;
}

size_t BlockStream::getBlockSize() const {
    return m_blockSize;
}

bool BlockStream::startReading() {
    if (m_started || m_finished || getBlockSize() == 0) {
        return false;
    }

    m_offset = m_nextOffset = 0;
    m_finished = false;
    m_started = true;
    return doStartReading();
}

void BlockStream::stopReading() {
    if (!m_started) {
        return;
    }
    doStopReading();
    m_finished = true;
}

size_t BlockStream::readNext(uint8_t* outputBuf) {
    if (!m_started || m_finished) {
        return 0;
    }

    m_offset = m_nextOffset;
    size_t numBytesRead = doReadNext(outputBuf);

    if (numBytesRead == 0) {
        m_finished = true;
        return 0;
    }

    m_nextOffset = m_offset + numBytesRead;
    return numBytesRead;
}

size_t BlockStream::getOffset() const {
    return m_offset;
}
