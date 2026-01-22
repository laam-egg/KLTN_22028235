#include <gtest/gtest.h>
#include "testing/core/fixtures/efe_test_data_fixture.h"
#include <iostream>

#include "efe/core/f_general_file_info.h"
#include "efe/core/f_byte_entropy_histogram.h"
#include "efe/core/f_string_extractor.h"
#include "efe/core/f_header_file_info.h"
#include "efe/core/f_section_info.h"
#include "efe/core/f_imports_info.h"
#include "efe/core/f_exports_info.h"
#include "efe/core/f_data_directories.h"
#include "efe/core/f_rich_header.h"
#include "efe/core/f_authenticode_signature.h"
#include "efe/core/f_pe_format_warnings.h"

inline constexpr feature_t const tolerateDecimalPlaces(int const decimalPlaces) {
    return std::pow(10.0, -decimalPlaces);
}

void testFeatureType(
    std::vector<feature_t> const& expectedFeatureVector,
    std::vector<feature_t> const& actualFeatureVector,
    char const* featureTypeName, size_t offset, size_t length,
    feature_t const tolerance
) {
    SCOPED_TRACE(testing::Message() << "while testing feature type: " << featureTypeName);
    feature_t const* expected = expectedFeatureVector.data() + offset;
    feature_t const* actual = actualFeatureVector.data() + offset;
    for (size_t i = 0; i < length; ++i) {
        SCOPED_TRACE(testing::Message() << "at index " << i);
        EXPECT_NEAR(expected[i], actual[i], tolerance);
    }
}

TEST_F(EFEFixtureTest, AllFeatures) {
    EMBER2024FeatureExtractor ef;

    PESampleReader sampleReader = this->createPESampleReader();
    PESample sample;
    std::vector<feature_t> actualFeatureVector;
    while (sampleReader.next(sample)) {
        #ifdef DEBUG
        if (sample.sha256 != "57d0068b412269f08af1496f316d809d6bf8313f789630676b736b1cfeef609c" && sample.sha256 != "6b33fa9a439a86f553f9f60e538ccabc857d2f308bc77c477c04a46552ade81f") {
            continue;
        }
        #endif // DEBUG
        SCOPED_TRACE(testing::Message() << "with sample: " << sample.sha256);
        std::error_code err;
        feature_t const* actualRawFeatureVector =
            ef.run(sample.fileContent.data(), sample.fileContent.size(), err);
        actualFeatureVector.assign(
            actualRawFeatureVector,
            actualRawFeatureVector + NUM_FEATURES
        );

        std::vector<feature_t> const& expectedFeatureVector =
            sample.featureVector;
        
        #define TEST_FT_WITH_TOLERANCE(FeatureTypeClass, tolerance) \
            { \
                char const* featureTypeName = FeatureTypeClass::getFeatureName(); \
                auto const offsetAndLength = ef.getFeatureTypeOffsetAndLength(featureTypeName); \
                size_t offset = std::get<0>(offsetAndLength); \
                size_t length = std::get<1>(offsetAndLength); \
                testFeatureType( \
                    expectedFeatureVector, \
                    actualFeatureVector, \
                    featureTypeName, \
                    offset, \
                    length, \
                    tolerance \
                ); \
            } (void)0 \
        
        #define TEST_FT(FeatureTypeClass) TEST_FT_WITH_TOLERANCE(FeatureTypeClass, tolerateDecimalPlaces(3))

        TEST_FT(GeneralFileInfo);
        TEST_FT(ByteEntropyHistogram);
        TEST_FT(StringExtractor);
        TEST_FT(HeaderFileInfo);
        TEST_FT_WITH_TOLERANCE(SectionInfo, 3.9);
        TEST_FT_WITH_TOLERANCE(ImportsInfo, 1.05);
        TEST_FT(ExportsInfo);
        TEST_FT(DataDirectories);
        TEST_FT(RichHeaderFeatureType);
        // TEST_FT(AuthenticodeSignature);
        // TEST_FT(PEFormatWarnings);
    }
}

// TEST_F(EFEFixtureTest, DummyGeneralFileInfo2) {
//     EXPECT_TRUE(true);
// }
