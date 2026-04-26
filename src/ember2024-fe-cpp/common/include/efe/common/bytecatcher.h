#ifndef ByteCatcher_INCLUDED
#define ByteCatcher_INCLUDED

#include <cstddef>
#include <cstdint>
#include <unordered_map>

class ByteCatcher {
private:
    std::unordered_map<size_t, uint8_t> offsetToByteMap;
    size_t maxOffsetToLookFor = 0;
    // size_t minOffsetToLookFor = 0;

public:
    ByteCatcher();

    void reset();

    void wantByteAtOffset(size_t offset);

    void start();

    void reduce(size_t bufOffset, uint8_t const* buf, size_t bufSize);

    void finalize();

    inline constexpr std::unordered_map<size_t, uint8_t> const& getOffsetToByteMap() const {
        return offsetToByteMap;
    }
};

#endif // ByteCatcher_INCLUDED
