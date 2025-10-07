#include "efe/core.h"
#include "efe/pefile.h"
#include "efe/common/logging.h"
#include "efe/common/memoryblockstream.h"

static inline constexpr size_t const BUF_SIZE = 1048576 * 4; // 4 MB

// #define LOG_FE_PROGRESS LOG_INFO
#define LOG_FE_PROGRESS(...)

EMBER2024FeatureExtractor::EMBER2024FeatureExtractor() :
    allFeatures(std::make_unique<AllFeatureTypes>()),

    dim{allFeatures->getMaxDim()},

    buf{std::make_unique<uint8_t[]>(BUF_SIZE)},

    output{std::make_unique<feature_t[]>(dim)}
{}

feature_t const* EMBER2024FeatureExtractor::run(uint8_t const* fileContent, size_t fileSize) {
    LOG_FE_PROGRESS("Parsing PE file...");
    PEFile peFile(fileContent, fileSize);

    LOG_FE_PROGRESS("Creating block stream...");
    MemoryBlockStream blockStream;
    blockStream.setup(fileContent, fileSize);
    blockStream.setBlockSize(BUF_SIZE);
    blockStream.startReading();
    size_t bufSize = 0;

    LOG_FE_PROGRESS("Starting feature engineering...");
    allFeatures->reset(output.get(), peFile);
    allFeatures->start(output.get(), peFile);

    while (0 != (
        bufSize = blockStream.readNext(buf.get())
    )) {
        LOG_FE_PROGRESS("... passing block at offset %zu", blockStream.getOffset());
        allFeatures->reduce(
            output.get(), peFile,
            blockStream.getOffset(),
            buf.get(),
            bufSize
        );
    }

    LOG_FE_PROGRESS("Finalizing feature engineering...");
    allFeatures->finalize(output.get(), peFile);

    blockStream.stopReading();

    return output.get();
}
