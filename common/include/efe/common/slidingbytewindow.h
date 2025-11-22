#ifndef BYTE_ACCUMULATOR_INCLUDED
#define BYTE_ACCUMULATOR_INCLUDED

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

using WithCompleteWindowCallback = std::function<void(uint8_t const* block, size_t blockSize)>;

class SlidingByteWindow {
private:
    std::vector<uint8_t> block;
    size_t windowSize;
    size_t step;
    WithCompleteWindowCallback callback;

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

    /**
     * Get the last block anyway, even if it is smaller than window size.
     */
    std::vector<uint8_t> const& getLastBlockAnyway() const;
};

#endif // BYTE_ACCUMULATOR_INCLUDED
