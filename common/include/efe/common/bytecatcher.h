#ifndef ByteCatcher_INCLUDED
#define ByteCatcher_INCLUDED

#include <cstddef>
#include <unordered_map>

class ByteCatcher {
private:
    std::unordered_map<size_t, char> offsetToByteMap;
    size_t maxOffsetToLookFor = 0;
    // size_t minOffsetToLookFor = 0;

public:
    ByteCatcher();

    void reset();

    void wantByteAtOffset(size_t offset);

    void start();

    void reduce(size_t bufOffset, char const* buf, size_t bufSize);

    void finalize();

    inline constexpr std::unordered_map<size_t, char> const& getOffsetToByteMap() const {
        return offsetToByteMap;
    }
};

#endif // ByteCatcher_INCLUDED
