#include "efe/core/all_feature_types.h"
#include "efe/common/logging.h"

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

static inline constexpr size_t const NUM_FEATURES = 2568;

#define DEBUG

AllFeatureTypes::AllFeatureTypes() :
    
    featureTypes{},

    dim{ 0 }
{
    #define F(className) featureTypes.emplace_back(std::make_unique<className>());
    F(GeneralFileInfo)
    F(ByteEntropyHistogram)
    F(StringExtractor)
    F(HeaderFileInfo)
    F(SectionInfo)
    F(ImportsInfo)
    F(ExportsInfo)
    F(DataDirectories)
    F(RichHeaderFeatureType)
    F(AuthenticodeSignature)
    F(PEFormatWarnings)
    #undef F

    for (auto const& ft : featureTypes) {
        dim += ft->getMaxDim();
    }

    #ifdef DEBUG
    if (dim != NUM_FEATURES) {
        LOG_FATAL_ERROR("Sum of feature types' dimensions (%zu) is not equal to the correct number of features (%zu)", dim, NUM_FEATURES);
    }
    #endif
}

char const* AllFeatureTypes::getName() const {
    return "all";
}

void AllFeatureTypes::reset(feature_t* output, PEFile const& peFile) {
    feature_t* vec = output;
    for (auto const& ft : featureTypes) {
        ft->reset(vec, peFile);
        vec += ft->getMaxDim();
    }
}

void AllFeatureTypes::start(feature_t* output, PEFile const& peFile) {
    // std::memset(output, 0, dim * sizeof(output[0]));
    feature_t* vec = output;
    for (auto const& ft : featureTypes) {
        ft->start(vec, peFile);
        vec += ft->getMaxDim();
    }
}

void AllFeatureTypes::reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) {
    feature_t* vec = output;
    for (auto const& ft : featureTypes) {
        ft->reduce(vec, peFile, bufOffset, buf, bufSize);
        vec += ft->getMaxDim();
    }
}

void AllFeatureTypes::finalize(feature_t* output, PEFile const& peFile) {
    feature_t* vec = output;
    for (auto const& ft : featureTypes) {
        ft->finalize(vec, peFile);
        vec += ft->getMaxDim();
    }
}

size_t AllFeatureTypes::getMaxDim() const {
    return dim;
}
