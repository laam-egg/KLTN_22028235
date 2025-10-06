#include "efe/core/f_section_info.h"
#include "efe/common/bytecounter.h"
#include "efe/common/shannonentropycalculator.h"
#include "efe/common/nop.h"
#include <algorithm>

#define DIM 224

// Bucket size for FeatureHasher
#define SECTION_SIZES_FH_BUCKET_SIZE 50
#define SECTION_VSIZES_FH_BUCKET_SIZE 50
#define SECTION_ENTROPY_FH_BUCKET_SIZE 50
#define CHARACTERISTICS_FH_BUCKET_SIZE 50
#define ENTRY_NAME_HASHED_FH_BUCKET_SIZE 10

class Offset {
public:
    #define ADD(NAME, VALUE) static constexpr size_t const NAME = VALUE;

    ADD(N_SECTIONS, 0)
    ADD(N_ZERO_SIZE, 1)
    ADD(N_EMPTY_NAME, 2)
    ADD(N_READABLE_AND_EXECUTABLE_SECTIONS, 3)
    ADD(N_WRITABLE_SECTIONS, 4)
    ADD(MAX_ENTROPIES, 5)
    ADD(MIN_ENTROPIES, 6)
    ADD(MAX_SIZE_RATIOS, 7)
    ADD(MIN_SIZE_RATIOS, 8)
    ADD(MAX_VSIZE_RATIOS, 9)
    ADD(MIN_VSIZE_RATIOS, 10)

    ADD(SECTION_SIZES_HASHED, 11)
    ADD(SECTION_VSIZES_HASHED, SECTION_SIZES_HASHED + SECTION_SIZES_FH_BUCKET_SIZE)
    ADD(SECTION_ENTROPY_HASHED, SECTION_VSIZES_HASHED + SECTION_VSIZES_FH_BUCKET_SIZE)
    ADD(CHARACTERISTICS_HASHED, SECTION_ENTROPY_HASHED + SECTION_ENTROPY_FH_BUCKET_SIZE)
    ADD(ENTRY_NAME_HASHED, CHARACTERISTICS_HASHED + CHARACTERISTICS_FH_BUCKET_SIZE)

    ADD(OVERLAY_SIZE, ENTRY_NAME_HASHED + ENTRY_NAME_HASHED_FH_BUCKET_SIZE)
    ADD(OVERLAY_SIZE_RATIO, OVERLAY_SIZE + 1)
    ADD(OVERLAY_ENTROPY, OVERLAY_SIZE_RATIO + 1)

    #undef ADD

    static_assert(
        OVERLAY_ENTROPY + 1 == DIM
    );
};










///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// SectionInfoFeatureHashers //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "efe/featurehasher.h"

class SectionInfoFeatureHashers {
private:
    FeatureHasher sectionSizeFH;
    FeatureHasher sectionVSizeFH;
    FeatureHasher sectionEntropyFH;
    FeatureHasher characteristicsFH;
    FeatureHasher entrySectionNameFH;
    feature_t* output;

public:
    SectionInfoFeatureHashers();
    void reset();
    void setOutput(feature_t* newOutput);
    void start();
    void reduceFromSection(PESection const& section);
    void reduceFromEntrySectionName(char const* entryName);
    void finalize();
};

SectionInfoFeatureHashers::SectionInfoFeatureHashers() {
    output = NULL;
}

void SectionInfoFeatureHashers::reset() {
    sectionSizeFH.reset();
    sectionVSizeFH.reset();
    sectionEntropyFH.reset();
    characteristicsFH.reset();
    entrySectionNameFH.reset();
}

void SectionInfoFeatureHashers::setOutput(feature_t* newOutput) {
    output = newOutput;
}

void SectionInfoFeatureHashers::start() {
    #define START_FH(fh, size, outputOffset) \
        fh.setSize(size); \
        fh.setOutput(output + outputOffset); \
        fh.start(); \
        nop()
    
    START_FH(sectionSizeFH, SECTION_SIZES_FH_BUCKET_SIZE, Offset::SECTION_SIZES_HASHED);
    START_FH(sectionVSizeFH, SECTION_VSIZES_FH_BUCKET_SIZE, Offset::SECTION_VSIZES_HASHED);
    START_FH(sectionEntropyFH, SECTION_ENTROPY_FH_BUCKET_SIZE, Offset::SECTION_ENTROPY_HASHED);
    START_FH(characteristicsFH, CHARACTERISTICS_FH_BUCKET_SIZE, Offset::CHARACTERISTICS_HASHED);
    START_FH(entrySectionNameFH, ENTRY_NAME_HASHED_FH_BUCKET_SIZE, Offset::ENTRY_NAME_HASHED);

    #undef START_FH
}

void SectionInfoFeatureHashers::reduceFromSection(PESection const& section) {
    char const* const sectionName = section.name.c_str();

    sectionSizeFH.reduce(sectionName, section.sizeRaw);
    sectionVSizeFH.reduce(sectionName, section.vsize);
    sectionEntropyFH.reduce(sectionName, section.entropy);
    for (
        char const* characteristicsString
        : SectionCharacteristic::getNamesByValue(section.characteristics)
    ) {
        characteristicsFH.reduce(characteristicsString, 1.0);
    }
}

void SectionInfoFeatureHashers::reduceFromEntrySectionName(char const* entryName) {
    entrySectionNameFH.reduce(entryName, 1.0);
}

void SectionInfoFeatureHashers::finalize() {
    sectionSizeFH.finalize();
    sectionVSizeFH.finalize();
    sectionEntropyFH.finalize();
    characteristicsFH.finalize();
    entrySectionNameFH.finalize();
    output = NULL;
}











///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////// SectionInfo /////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

char const* SectionInfo::getName() const {
    return "section";
}

void SectionInfo::reset(feature_t* output, PEFile const& peFile) {
    // pass;
}

void SectionInfo::start(feature_t* output, PEFile const& peFile) {
    if (!peFile.isPEFile()) {
        std::memset(output, 0, DIM * sizeof(feature_t));
        return;
    }

    uint8_t const* overlayBuf = NULL;
    size_t overlaySize = 0;
    peFile.getOverlayBytes(&overlayBuf, &overlaySize);
    entropy_t overlayEntropy = 0;
    if (overlayBuf != NULL && overlaySize > 0) {
        ByteCounter overlayByteCounter;
        overlayByteCounter.reset();
        overlayByteCounter.start();
        overlayByteCounter.reduce(overlayBuf, overlaySize);
        overlayByteCounter.finalize();
        overlayEntropy = calculateShannonEntropy(
            overlayByteCounter.getTotalNumBytes(),
            overlayByteCounter.getByteCountsArray(),
            256
        );
    }

    size_t fileSize = peFile.getFileSize();
    feature_t overlaySizeRatio = fileSize == 0 ? 0.0 : static_cast<feature_t>(overlaySize) / fileSize;

    entropy_t maxEntropy = overlayEntropy;
    entropy_t minEntropy = overlayEntropy;
    feature_t maxSizeRatio = overlaySizeRatio;
    feature_t minSizeRatio = overlaySizeRatio;
    feature_t maxVSizeRatio = 0;
    feature_t minVSizeRatio = 0;

    PESection const* pEntrySection = nullptr;
    PESection const* pPotentialEntrySection = nullptr;
    size_t entrypointRVA = peFile.getEntrypointRVA();
    std::vector<PESection> const& sections = peFile.getSections();
    size_t numSectionsZeroSize = 0;
    size_t numSectionsEmptyName = 0;
    size_t numSectionsReadableAndExecutable = 0;
    size_t numSectionsWritable = 0;
    SectionInfoFeatureHashers fh;
    fh.reset();
    fh.setOutput(output);
    fh.start();
    for (auto const& section : sections) {
        maxEntropy = std::max(maxEntropy, static_cast<entropy_t>(section.entropy));
        minEntropy = std::min(minEntropy, static_cast<entropy_t>(section.entropy));
        maxSizeRatio = std::max(maxSizeRatio, static_cast<decltype(maxSizeRatio)>(section.sizeRatio));
        minSizeRatio = std::min(minSizeRatio, static_cast<decltype(minSizeRatio)>(section.sizeRatio));
        maxVSizeRatio = std::max(maxVSizeRatio, static_cast<decltype(maxVSizeRatio)>(section.vsizeRatio));
        minVSizeRatio = std::min(minVSizeRatio, static_cast<decltype(minVSizeRatio)>(section.vsizeRatio));

        fh.reduceFromSection(section);

        if (section.sizeRaw == 0) {
            ++numSectionsZeroSize;
        }
        if (section.name.empty()) {
            ++numSectionsEmptyName;
        }
        if (section.hasCharacteristic(SectionCharacteristic::MEM_WRITE)) {
            ++numSectionsWritable;
        }
        if (section.hasCharacteristic(SectionCharacteristic::MEM_EXECUTE)) {
            if (section.hasCharacteristic(SectionCharacteristic::MEM_READ)) {
                ++numSectionsReadableAndExecutable;
                if (!pPotentialEntrySection) pPotentialEntrySection = &section;
            } else {
                // An executable but not readable section? Very suspicious! Memorize it right away!
                pPotentialEntrySection = &section;
            }
        }

        if (section.containsRVA(entrypointRVA)) {
            pEntrySection = &section; // find pEntry section the usual way
        }
    }

    // If the entrypointRVA is NOT in any section,
    // we can't find pEntrySection the usual way;
    // so we set pEntrySection to the first (most suspicious)
    // executable section we've found, to compensate.
    if (pEntrySection == nullptr) {
        pEntrySection = pPotentialEntrySection;
    }

    if (pEntrySection != nullptr) {
        fh.reduceFromEntrySectionName(pEntrySection->name.c_str());
    }
    fh.finalize();

    output[Offset::N_SECTIONS] = sections.size();
    output[Offset::N_ZERO_SIZE] = numSectionsZeroSize;
    output[Offset::N_EMPTY_NAME] = numSectionsEmptyName;
    output[Offset::N_READABLE_AND_EXECUTABLE_SECTIONS] = numSectionsReadableAndExecutable;
    output[Offset::N_WRITABLE_SECTIONS] = numSectionsWritable;
    output[Offset::MAX_ENTROPIES] = maxEntropy;
    output[Offset::MIN_ENTROPIES] = minEntropy;
    output[Offset::MAX_SIZE_RATIOS] = maxSizeRatio;
    output[Offset::MIN_SIZE_RATIOS] = minSizeRatio;
    output[Offset::MAX_VSIZE_RATIOS] = maxVSizeRatio;
    output[Offset::MIN_VSIZE_RATIOS] = minVSizeRatio;
    // Features residing from Offset::SECTION_SIZES_HASHED to Offset::ENTRY_NAME_HASHED
    // have been assigned by fh!
    output[Offset::OVERLAY_SIZE] = overlaySize;
    output[Offset::OVERLAY_SIZE_RATIO] = overlaySizeRatio;
    output[Offset::OVERLAY_ENTROPY] = overlayEntropy;
}

void SectionInfo::reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) {
    // pass
}

void SectionInfo::finalize(feature_t* output, PEFile const& peFile) {
    // pass
}

size_t SectionInfo::getMaxDim() const {
    return DIM;
}
