#include "efe/core.h"
#include "efe/pefile.h"
#include "efe/common/logging.h"
#include "efe/common/memoryblockstream.h"
#include <mio/mmap.hpp>

static inline constexpr size_t const BUF_SIZE = 1048576 * 4; // 4 MB

// #define LOG_FE_PROGRESS LOG_INFO
#define LOG_FE_PROGRESS(...)

EMBER2024FeatureExtractor::EMBER2024FeatureExtractor() :
    allFeatures(std::make_unique<AllFeatureTypes>()),

    dim{allFeatures->getMaxDim()},

    buf{std::make_unique<uint8_t[]>(BUF_SIZE)},

    output{std::make_unique<feature_t[]>(dim)}
{}

feature_t const* EMBER2024FeatureExtractor::run(
    uint8_t const* fileContent,
    size_t fileSize,
    std::error_code& errorCode
) {
    errorCode.clear();
    // Currently this function never fails.
    // But it might in the future.

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

feature_t const* EMBER2024FeatureExtractor::run(
    std::filesystem::path filePath,
    std::error_code& errorCode
) {
    errorCode.clear();

    #define FAIL_ERRC(err) { errorCode = err; return nullptr; } (void)0
    #define FAIL(err) FAIL_ERRC(std::make_error_code(err))
    #define FORWARD_SYSTEM_ERROR(exc) FAIL_ERRC(exc.code())
    #define CHECK_FAILURE() if (errorCode) { return nullptr; } (void)0

    LOG_FE_PROGRESS("Checking path...");
    filePath = std::filesystem::absolute(filePath);
    if (!std::filesystem::exists(filePath)) {
        FAIL(std::errc::no_such_file_or_directory);
    }

    LOG_FE_PROGRESS("Checking file size...");
    auto const fileSizeFromSystem = std::filesystem::file_size(filePath);
    if (fileSizeFromSystem == 0) {
        FAIL(std::errc::no_message);
    }

    LOG_FE_PROGRESS("Mapping the file onto memory...");
    mio::mmap_source mmap;
    try {
        mmap = mio::make_mmap_source(filePath.native(), 0, mio::map_entire_file, errorCode);
    } catch (std::system_error const& e) {
        FORWARD_SYSTEM_ERROR(e);
    }
    CHECK_FAILURE();

    LOG_FE_PROGRESS("Calling actual feature extractor...");
    uint8_t const* const fileContent = reinterpret_cast<uint8_t const*>(mmap.data());
    size_t const fileSize = mmap.size();
    if (fileSizeFromSystem != fileSize) {
        FAIL(std::errc::message_size);
    }
    feature_t const* featureVector = this->run(fileContent, fileSize, errorCode);
    CHECK_FAILURE();

    #undef CHECK_FAILURE
    #undef FORWARD_SYSTEM_ERROR
    #undef FAIL
    #undef FAIL_ERRC

    return featureVector;
}
