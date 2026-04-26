#include <gtest/gtest.h>
#include <memory>
#include <cstring>
#include "efe/common/slidingbytewindow.h"

class SlidingByteWindowFixtureTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

    SlidingByteWindow runSlidingByteWindowTest_OneReduction(
        std::string const& input,
        size_t windowSize, size_t step,
        std::vector<std::vector<uint8_t>> const& expectedBlocks
    ) {
        return runSlidingByteWindowTest_OneReduction(
            std::vector<uint8_t>(input.begin(), input.end()),
            windowSize, step,
            expectedBlocks
        );
    }

    SlidingByteWindow runSlidingByteWindowTest_OneReduction(
        std::vector<uint8_t> const& input,
        size_t windowSize, size_t step,
        std::vector<std::vector<uint8_t>> const& expectedBlocks
    ) {
        SCOPED_TRACE(testing::Message() << "windowSize=" << windowSize << ", step=" << step);
        SlidingByteWindow sbw;
        sbw.setWindowSize(windowSize);
        sbw.setStep(step);

        size_t callCount = 0;
        auto const callback = [&](uint8_t const* block, size_t blockSize) {
            SCOPED_TRACE(testing::Message() << "callback call #" << callCount << ", blockSize=" << blockSize << ", block=" << std::string(reinterpret_cast<char const*>(block), blockSize));
            EXPECT_LT(callCount, expectedBlocks.size());
            EXPECT_EQ(blockSize, windowSize);

            for (size_t i = 0; i < windowSize; ++i) {
                SCOPED_TRACE(testing::Message() << "at byte index " << i);
                EXPECT_EQ(block[i], expectedBlocks[callCount][i]);
            }

            ++callCount;
        };

        sbw.setCallback(callback);
        sbw.start();
        sbw.reduce(0, input.data(), input.size());
        sbw.finalize();

        EXPECT_EQ(callCount, expectedBlocks.size());

        return sbw;
    }
};

TEST_F(SlidingByteWindowFixtureTest, SlidingByteWindowTest1a) {
    this->runSlidingByteWindowTest_OneReduction(
        "abcdefghij",
        4,
        2,
        {
            { 'a', 'b', 'c', 'd' },
            { 'c', 'd', 'e', 'f' },
            { 'e', 'f', 'g', 'h' },
            { 'g', 'h', 'i', 'j' }
        }
    );
}

TEST_F(SlidingByteWindowFixtureTest, SlidingByteWindowTest1b) {
    this->runSlidingByteWindowTest_OneReduction(
        "abcdefghijklmnopqrstuvwxy",
        5,
        3,
        {
            { 'a', 'b', 'c', 'd', 'e' },
            { 'd', 'e', 'f', 'g', 'h' },
            { 'g', 'h', 'i', 'j', 'k' },
            { 'j', 'k', 'l', 'm', 'n' },
            { 'm', 'n', 'o', 'p', 'q' },
            { 'p', 'q', 'r', 's', 't' },
            { 's', 't', 'u', 'v', 'w' }
        }
    );
}

TEST_F(SlidingByteWindowFixtureTest, SlidingByteWindowTest1c) {
    SlidingByteWindow const sbw = this->runSlidingByteWindowTest_OneReduction(
        "abcdefgh",
        10,
        5,
        {}
    );

    EXPECT_EQ("abcdefgh", std::string(
        reinterpret_cast<char const*>(sbw.getLastBlockAnyway().data()),
        sbw.getLastBlockAnyway().size()
    ));
}
