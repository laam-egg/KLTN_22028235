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

    m_offset = 0;
    m_finished = false;
    m_started = true;
    return doStartReading();
}

void BlockStream::stopReading() {
    if (!m_started) {
        return;
    }
    doStopReading();
}

size_t BlockStream::readNext(char* buf) {
    if (!m_started || m_finished) {
        return 0;
    }

    size_t numBytesRead = doReadNext(buf);

    if (numBytesRead == 0) {
        m_finished = true;
        return 0;
    }

    m_offset += numBytesRead;
    return numBytesRead;
}

size_t BlockStream::getOffset() const {
    return m_offset;
}
