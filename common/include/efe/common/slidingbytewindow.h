#ifndef BYTE_ACCUMULATOR_INCLUDED
#define BYTE_ACCUMULATOR_INCLUDED

#include <cstddef>
#include <cstdint>
#include <functional>

using WithCompleteWindowCallback = std::function<void(uint8_t* firstSegment, size_t firstSegmentSize, uint8_t* secondSegment, size_t secondSegmentSize)>;

class SlidingByteWindow {
private:
    uint8_t* window;
    size_t windowSize;
    size_t step;
    size_t filledPartBeginPos;
    size_t filledPartSize;
    WithCompleteWindowCallback callback;

    void deallocateEverything();

    void callTheCallback() noexcept;

public:
    SlidingByteWindow();
    ~SlidingByteWindow();

    void reset();

    void setWindowSize(size_t newWindowSize);

    void setStep(size_t newStep);

    void setCallback(WithCompleteWindowCallback newCallback);

    void start();

    void reduce(size_t bufOffset, uint8_t const* buf, size_t bufSize);

    void finalize();
};

#endif // BYTE_ACCUMULATOR_INCLUDED
