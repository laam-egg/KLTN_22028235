#ifndef BYTECOUNTER_INCLUDED
#define BYTECOUNTER_INCLUDED

#include <cstddef>
#include <cstdint>

class ByteCounter {
private:
    size_t totalNumBytes = 0;
    size_t byteCounts[256];

public:
    ByteCounter();

    void reset();

    void start();

    void reduce(uint8_t const* buf, size_t bufSize);

    void finalize();

    inline constexpr size_t getTotalNumBytes() const { return totalNumBytes; }

    inline constexpr size_t const* getByteCountsArray() const { return byteCounts; }

    /**
     * Only use this after finalize(). Use at your own risk.
     * If you update the resulting array, subsequent calls
     * to getTotalNumBytes() or this method could be
     * surprising - since the underlying buffer has been
     * modified by you.
     */
    inline constexpr size_t* unsafe_getModifiableCountsArray() { return byteCounts; }
};

#endif // BYTECOUNTER_INCLUDED
