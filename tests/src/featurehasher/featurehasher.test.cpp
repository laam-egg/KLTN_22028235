#include <gtest/gtest.h>
#include <memory>
#include <cmath>
#include "efe/featurehasher.h"

class FeatureHasherFixtureTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

constexpr int const FEATURE_DECIMAL_PLACES_FOR_COMPROMISE = 3;
static feature_t const FEATURE_TOLERANCE = std::pow(10.0, -FEATURE_DECIMAL_PLACES_FOR_COMPROMISE);

/**
 * From:
 * https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/feature_extraction/_hash.py#L89C1-L94C64
 */
TEST_F(FeatureHasherFixtureTest, FHTest1a) {
    constexpr size_t const N = 10;
    constexpr size_t const NUM_SAMPLES = 2;
    constexpr size_t const OUTPUT_LENGTH = N * NUM_SAMPLES;

    auto actualOutput = std::make_unique<feature_t[]>(OUTPUT_LENGTH);
    memset(reinterpret_cast<void*>(actualOutput.get()), 0, OUTPUT_LENGTH * sizeof(feature_t));

    FeatureHasher fh;

    fh.reset();
    fh.setSize(N);
    fh.setOutput(actualOutput.get());
    fh.setMultipleSamples(true);
    fh.start();

    fh.reduce("dog", 1);
    fh.reduce("cat", 2);
    fh.reduce("elephant", 4);
    fh.finalizeSample();

    fh.reduce("dog", 2);
    fh.reduce("run", 5);
    fh.finalizeSample();

    fh.finalize();

    feature_t const expectedOutput[OUTPUT_LENGTH] = {
        0.0,  0.0, -4.0, -1.0,  0.0,  0.0,  0.0,  0.0,  0.0,  2.0,
        0.0,  0.0,  0.0, -2.0, -5.0,  0.0,  0.0,  0.0,  0.0,  0.0
    };

    for (size_t i = 0; i < OUTPUT_LENGTH; ++i) {
        SCOPED_TRACE(testing::Message() << "at index " << i);
        EXPECT_NEAR(expectedOutput[i], actualOutput.get()[i], FEATURE_TOLERANCE);
    }
}


/**
 * From:
 * https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/feature_extraction/_hash.py#L99C1-L105C54
 */
TEST_F(FeatureHasherFixtureTest, FHTest1b) {
    constexpr size_t const N = 8;
    constexpr size_t const NUM_SAMPLES = 3;
    constexpr size_t const OUTPUT_LENGTH = N * NUM_SAMPLES;

    auto actualOutput = std::make_unique<feature_t[]>(OUTPUT_LENGTH);
    memset(reinterpret_cast<void*>(actualOutput.get()), 0, OUTPUT_LENGTH * sizeof(feature_t));

    FeatureHasher fh;

    fh.reset();
    fh.setSize(N);
    fh.setOutput(actualOutput.get());
    fh.setMultipleSamples(true);
    fh.start();

    fh.reduce("dog", 1);
    fh.reduce("cat");
    fh.reduce("snake");
    fh.finalizeSample();

    fh.reduce("snake");
    fh.reduce("dog");
    fh.finalizeSample();

    fh.reduce("cat");
    fh.reduce("bird", 1);
    fh.finalizeSample();

    fh.finalize();

    feature_t const expectedOutput[OUTPUT_LENGTH] = {
        0.0,  0.0,  0.0, -1.0,  0.0, -1.0,  0.0,  1.0,
        0.0,  0.0,  0.0, -1.0,  0.0, -1.0,  0.0,  0.0,
        0.0, -1.0,  0.0,  0.0,  0.0,  0.0,  0.0,  1.0
    };

    for (size_t i = 0; i < OUTPUT_LENGTH; ++i) {
        SCOPED_TRACE(testing::Message() << "at index " << i);
        EXPECT_NEAR(expectedOutput[i], actualOutput.get()[i], FEATURE_TOLERANCE);
    }
}

/**
 * From <project_root>/featurehasher/README.md
 */
TEST_F(FeatureHasherFixtureTest, FHTest2) {
    constexpr size_t const N = 50;
    constexpr size_t const NUM_SAMPLES = 1;
    constexpr size_t const OUTPUT_LENGTH = N * NUM_SAMPLES;

    auto actualOutput = std::make_unique<feature_t[]>(OUTPUT_LENGTH);
    memset(reinterpret_cast<void*>(actualOutput.get()), 0, OUTPUT_LENGTH * sizeof(feature_t));

    FeatureHasher fh;

    fh.reset();
    fh.setSize(N);
    fh.setOutput(actualOutput.get());
    fh.start();

    fh.reduce("Abc");
    fh.reduce("DEF", 12.3);

    // No need to
    //     fh.finalizeSample();
    // since multipleSamples = false by default

    fh.finalize();

    feature_t const expectedOutput[OUTPUT_LENGTH] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12.3,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
    };

    for (size_t i = 0; i < OUTPUT_LENGTH; ++i) {
        SCOPED_TRACE(testing::Message() << "at index " << i);
        EXPECT_NEAR(expectedOutput[i], actualOutput.get()[i], FEATURE_TOLERANCE);
    }
}
