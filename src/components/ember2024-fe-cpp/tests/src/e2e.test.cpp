#include <gtest/gtest.h>

class E2EFixtureTest : public ::testing::Test {
protected:
    bool m = false;

    void SetUp() override {
        // Code to run before each test in this fixture
        m = true;
    }
    void TearDown() override {
        // Code to run after each test in this fixture
        m = false;
    }
};

TEST_F(E2EFixtureTest, Dummy) {
    // Test logic using fixture members
    EXPECT_TRUE(true);
    EXPECT_TRUE(m);
}
