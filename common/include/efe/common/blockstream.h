#ifndef BlockStream_INCLUDED
#define BlockStream_INCLUDED

#include <cstddef>

class BlockStream {
public:
    void setBlockSize(size_t blockSize);

    size_t getBlockSize() const;

    bool startReading();

    void stopReading();

    size_t readNext(char* buf);

    size_t getOffset() const;

protected:
    virtual bool doStartReading() = 0;
    virtual void doStopReading() = 0;
    virtual size_t doReadNext(char* buf) = 0;

private:
    size_t m_blockSize = 0;
    size_t m_offset = 0;
    bool m_started = false;
    bool m_finished = false;
};

#endif // BlockStream_INCLUDED
