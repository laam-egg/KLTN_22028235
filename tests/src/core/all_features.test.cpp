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

#include <type_traits>

template<class> inline constexpr bool always_false_v = false;

template<typename T>
inline void EXPECT_REAL_EQ(T a, T b) {
    if constexpr (std::is_same_v<T, float>) {
        EXPECT_FLOAT_EQ(a, b);
    } else if constexpr (std::is_same_v<T, double>) {
        EXPECT_DOUBLE_EQ(a, b);
    } else {
        static_assert(always_false_v<T>, "T is not float or double? Then what the hell is it?");
    }
}

constexpr int const FEATURE_DECIMAL_PLACES_FOR_COMPROMISE = 3;
static feature_t const FEATURE_TOLERANCE = std::pow(10.0, -FEATURE_DECIMAL_PLACES_FOR_COMPROMISE);

void testFeatureType(
    std::vector<feature_t> const& expectedFeatureVector,
    std::vector<feature_t> const& actualFeatureVector,
    char const* featureTypeName, size_t offset, size_t length
) {
    SCOPED_TRACE(testing::Message() << "while testing feature type " << featureTypeName);
    feature_t const* expected = expectedFeatureVector.data() + offset;
    feature_t const* actual = actualFeatureVector.data() + offset;
    for (size_t i = 0; i < length; ++i) {
        SCOPED_TRACE(testing::Message() << "at index " << i);
        EXPECT_NEAR(expected[i], actual[i], FEATURE_TOLERANCE);
    }
}

TEST_F(EFEFixtureTest, AllFeatures) {
    EMBER2024FeatureExtractor ef;

    PESampleReader sampleReader = this->createPESampleReader();
    PESample sample;
    std::vector<feature_t> actualFeatureVector;
    while (sampleReader.next(sample)) {
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
        
        #define TEST_FT(FeatureTypeClass) \
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
                    length \
                ); \
            } (void)0 \

        TEST_FT(GeneralFileInfo);
        // TEST_FT(ByteEntropyHistogram);
        // TEST_FT(StringExtractor);
        TEST_FT(HeaderFileInfo);
        // TEST_FT(SectionInfo);
        // TEST_FT(ImportsInfo);
        // TEST_FT(ExportsInfo);
        // TEST_FT(DataDirectories);
        // TEST_FT(RichHeaderFeatureType);
        // TEST_FT(AuthenticodeSignature);
        // TEST_FT(PEFormatWarnings);
    }
}

// TEST_F(EFEFixtureTest, DummyGeneralFileInfo2) {
//     EXPECT_TRUE(true);
// }
