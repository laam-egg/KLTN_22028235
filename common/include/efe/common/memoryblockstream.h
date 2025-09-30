#ifndef MemoryBlockStream_INCLUDED
#define MemoryBlockStream_INCLUDED

#include "efe/common/blockstream.h"

class MemoryBlockStream final : public BlockStream {
public:
    void setup(char const* buf, size_t bufSize);

protected:
    virtual bool doStartReading() override;
    virtual void doStopReading() override;
    virtual size_t doReadNext(char* buf) override;

private:
    char const* m_buf = NULL;
    size_t m_bufSize = 0;
};

#endif // MemoryBlockStream_INCLUDED
