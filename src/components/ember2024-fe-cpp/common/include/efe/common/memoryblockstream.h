#ifndef MemoryBlockStream_INCLUDED
#define MemoryBlockStream_INCLUDED

#include "efe/common/blockstream.h"
#include <cstdint>

class MemoryBlockStream final : public BlockStream {
public:
    void setup(uint8_t const* buf, size_t bufSize);

protected:
    virtual bool doStartReading() override;
    virtual void doStopReading() override;
    virtual size_t doReadNext(uint8_t* buf) override;

private:
    uint8_t const* m_buf = NULL;
    size_t m_bufSize = 0;
};

#endif // MemoryBlockStream_INCLUDED
