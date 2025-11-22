#include <gtest/gtest.h>
#include <memory>
#include <cmath>
#include "efe/common/bytecounter.h"

class ByteCounterFixtureTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(ByteCounterFixtureTest, ByteCounterTest1a) {
    #define BYTE_RANGE 256
    uint8_t fullRange[BYTE_RANGE];
    for (size_t i = 0; i < BYTE_RANGE; ++i) {
        fullRange[i] = static_cast<uint8_t>(i);
    }
    ByteCounter bc;
    bc.reset();
    bc.start();
    bc.reduce(fullRange, BYTE_RANGE);
    bc.reduce(fullRange, BYTE_RANGE);
    bc.reduce(fullRange, BYTE_RANGE);
    bc.finalize();

    EXPECT_EQ(3 * BYTE_RANGE, bc.getTotalNumBytes());
    for (size_t i = 0; i < BYTE_RANGE; ++i) {
        SCOPED_TRACE(testing::Message() << "at index " << i);
        EXPECT_EQ(3, bc.getByteCountsArray()[i]);
    }
    #undef BYTE_RANGE
}

TEST_F(ByteCounterFixtureTest, ByteCounterTest1b) {
    #define BYTE_RANGE 3
    uint8_t range[BYTE_RANGE];

    range[0] = 46;
    range[1] = 0;
    range[2] = 46;
    static_assert(BYTE_RANGE >= 3);

    ByteCounter bc;
    bc.reset();
    bc.start();
    bc.reduce(range, 2);
    bc.reduce(range + 2, 1);
    bc.reduce(range, 1);
    bc.reduce(range + 2, 1);
    bc.reduce(range + 1, 1);
    bc.finalize();

    EXPECT_EQ(2 * BYTE_RANGE, bc.getTotalNumBytes());
    EXPECT_EQ(4, bc.getByteCountsArray()[46]);
    EXPECT_EQ(2, bc.getByteCountsArray()[0]);
    for (size_t i = 0; i < BYTE_RANGE; ++i) {
        if (i == 0 || i == 46) continue;
        SCOPED_TRACE(testing::Message() << "at index " << i);
        EXPECT_EQ(0, bc.getByteCountsArray()[i]);
    }

    #undef BYTE_RANGE
}
