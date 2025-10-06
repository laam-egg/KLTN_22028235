#include "efe/core.h"
#include "efe/pefile.h"
#include "efe/common/logging.h"
#include "efe/common/memoryblockstream.h"

static inline constexpr size_t const BUF_SIZE = 16384;

EMBER2024FeatureExtractor::EMBER2024FeatureExtractor() :
    allFeatures(std::make_unique<AllFeatureTypes>()),

    dim{allFeatures->getMaxDim()},

    buf{std::make_unique<uint8_t[]>(BUF_SIZE)},

    output{std::make_unique<feature_t[]>(dim)}
{}

feature_t const* EMBER2024FeatureExtractor::run(uint8_t const* fileContent, size_t fileSize) {
    PEFile peFile(fileContent, fileSize);

    MemoryBlockStream blockStream;
    blockStream.setup(fileContent, fileSize);
    blockStream.setBlockSize(BUF_SIZE);
    blockStream.startReading();
    size_t bufSize = 0;

    allFeatures->reset(output.get(), peFile);
    allFeatures->start(output.get(), peFile);

    while (0 != (
        bufSize = blockStream.readNext(buf.get())
    )) {
        allFeatures->reduce(
            output.get(), peFile,
            blockStream.getOffset(),
            buf.get(),
            bufSize
        );
    }

    allFeatures->finalize(output.get(), peFile);

    blockStream.stopReading();

    return output.get();
}
