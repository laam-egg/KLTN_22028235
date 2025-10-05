#include "efe/core/f_string_extractor.h"
#include "efe/common/logging.h"
#include "efe/common/meth.h"

std::regex const StringExtractor::READABLE_STRING_REGEX = std::regex("[\\x20-\\x7f]{5,}");

// TODO: Regex matching could be faster
// with RE2 or Hyperscan.
std::regex const StringExtractor::INTERESTING_STRING_REGEXES[] = {
#define IGNORECASE std::regex::icase
#define ADD_FINAL(...) std::regex(__VA_ARGS__)
#define ADD(...) std::regex(__VA_ARGS__),

    ADD(".click", IGNORECASE)
    ADD("/EmbeddedFile")
    ADD("/FlateDecode")
    ADD("/URI")
    ADD("/bin/")
    ADD("/dev/")
    ADD("/proc/")
    ADD("/tmp/")
    ADD("/usr/")
    ADD("<script", IGNORECASE)
    ADD("Invoke-Command")
    ADD("Invoke-Expression")
    ADD("Start-process")
    ADD("base64", IGNORECASE)
    ADD("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/")
    ADD("[13][a-km-zA-HJ-NP-Z1-9]{25,34}")
    ADD("cache", IGNORECASE)
    ADD("certificate", IGNORECASE)
    ADD("clipboard", IGNORECASE)
    ADD("command", IGNORECASE)
    ADD("connect", IGNORECASE)
    ADD("cookie", IGNORECASE)
    ADD("create", IGNORECASE)
    ADD("crypt")
    ADD("debug", IGNORECASE)
    ADD("decode", IGNORECASE)
    ADD("delete", IGNORECASE)
    ADD("desktop", IGNORECASE)
    ADD("directory", IGNORECASE)
    ADD("disk", IGNORECASE)
    ADD("!This program ")
    ADD("download", IGNORECASE)
    // https://github.com/FutureComputing4AI/EMBER2024/issues/7
    ADD("\\b(?:[0-9A-Fa-f]{2}[:-]){5}(?:[0-9A-Fa-f]{2})\\b")
    ADD("encode", IGNORECASE)
    ADD("enum", IGNORECASE)
    ADD("environment", IGNORECASE)
    ADD("exit", IGNORECASE)
    ADD("file", IGNORECASE)
    ADD("\\bC:/")
    ADD("ftp:", IGNORECASE)
    ADD("GET /", IGNORECASE)
    ADD("hidden", IGNORECASE)
    ADD("hostname", IGNORECASE)
    ADD("html", IGNORECASE)
    ADD("HTTP/", IGNORECASE)
    ADD("http://", IGNORECASE)
    ADD("https://", IGNORECASE)
    ADD("install", IGNORECASE)
    ADD("internet", IGNORECASE)
    ADD("\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b")
    ADD("\\b(?:[A-Fa-f0-9]{1,4}:){7}[A-Fa-f0-9]{1,4}\\b|\\b(?:[A-Fa-f0-9]{1,4}:){1,7}:\\b|\\b:[A-Fa-f0-9]{1,4}(?::[A-Fa-f0-9]{1,4}){1,6}\\b")
    ADD("javascript", IGNORECASE)
    ADD("keyboard", IGNORECASE)
    ADD("\\b(?:[0-9A-Fa-f]{2}[:-]){5}(?:[0-9A-Fa-f]{2})\\b")
    ADD("memory", IGNORECASE)
    ADD("module", IGNORECASE)
    ADD("mutex", IGNORECASE)
    ADD("onclick", IGNORECASE)
    ADD("password", IGNORECASE)
    ADD("POST /", IGNORECASE)
    ADD("powershell", IGNORECASE)
    ADD("privilege", IGNORECASE)
    ADD("process", IGNORECASE)
    ADD("\\b(?:KHEY_|KHLM|HKCU)")
    ADD("remote", IGNORECASE)
    ADD("resource", IGNORECASE)
    ADD("security", IGNORECASE)
    ADD("service", IGNORECASE)
    ADD("shell", IGNORECASE)
    ADD("snapshot", IGNORECASE)
    ADD("system", IGNORECASE)
    ADD("thread", IGNORECASE)
    ADD("token", IGNORECASE)
    ADD("\\b(?:http|https|ftp):\\/\\/[a-zA-Z0-9-._~:?#[\\]@!$&'()*+,;=]+")
    ADD("User-Agent", IGNORECASE)
    ADD("wallet", IGNORECASE)
    ADD_FINAL("window", IGNORECASE)

#undef ADD
#undef ADD_FINAL
#undef IGNORECASE
};

StringExtractor::StringExtractor() {
    NUM_INTERESTING_STRING_REGEXES =
        sizeof(StringExtractor::INTERESTING_STRING_REGEXES)
        / sizeof(StringExtractor::INTERESTING_STRING_REGEXES[0]);
    
    if (NUM_INTERESTING_STRING_REGEXES >= 256) {
        LOG_ERROR("The algorithm needs to be rewritten!");
        // Because in this case we can no longer abuse ByteCounter (below, search for the word "abuse")
    }
}

char const* StringExtractor::getName() const {
    return "strings";
}

void StringExtractor::reset(feature_t* output, PEFile const& peFile) {
    sumReadableStringLengths = 0;
    numReadableStrings = 0;
    readableByteCounter.reset();
    interestingStringCounter.reset();
}

void StringExtractor::start(feature_t* output, PEFile const& peFile) {
    readableByteCounter.start();
    interestingStringCounter.start();
}

void StringExtractor::reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) {
    char const* charBuf = reinterpret_cast<char const*>(buf);
    auto begin = std::cregex_iterator(charBuf, charBuf + bufSize, READABLE_STRING_REGEX);
    auto end = std::cregex_iterator();
    for (auto it = begin; it != end; ++it) {
        char const* matchBuf = charBuf + it->position();
        size_t matchLength = it->length();

        sumReadableStringLengths += matchLength;
        numReadableStrings += 1;

        readableByteCounter.reduce(reinterpret_cast<uint8_t const*>(matchBuf), matchLength);

        for (size_t i = 0; i < NUM_INTERESTING_STRING_REGEXES; ++i) {
            // Check if the matched string (i.e. the readable string just found)
            // matches any of the interesting regexes
            if (std::regex_search(matchBuf, matchBuf + matchLength, INTERESTING_STRING_REGEXES[i])) {
                // An abuse of ByteCounter - thanks to the fact that NUM_INTERESTING_STRING_REGEXES < 256
                uint8_t interestingStringRegexIndex = i;
                interestingStringCounter.reduce(&interestingStringRegexIndex, 1);
            }
        }
    }
}

void StringExtractor::finalize(feature_t* output, PEFile const& peFile) {
    readableByteCounter.finalize();
    interestingStringCounter.finalize();

    register size_t outputOffset = 0;

    // :: numstrings
    output[outputOffset] = numReadableStrings;
    outputOffset += 1;

    // :: avlength
    // average readable string length
    output[outputOffset] = numReadableStrings == 0 ? 0 : (
        sumReadableStringLengths / numReadableStrings
    );
    outputOffset += 1;

    // :: printables
    size_t totalNumReadableBytes = readableByteCounter.getTotalNumBytes();
    output[outputOffset] = (feature_t) totalNumReadableBytes;
    outputOffset += 1;

    // :: printabledist / hist_divisor
    // readable distribution/histogram
    #define FIRST_READABLE_BYTE 0x20
    #define LAST_READABLE_BYTE 0x7f
    size_t const* const readableHistogram = readableByteCounter.getByteCountsArray() + FIRST_READABLE_BYTE;
    constexpr size_t const readableHistogramSize = LAST_READABLE_BYTE + 1 - FIRST_READABLE_BYTE;
    feature_t* const outputReadableHistogram = output + outputOffset;
    arrayCopy(outputReadableHistogram, readableHistogram, readableHistogramSize);
    feature_t invTotal = totalNumReadableBytes == 0 ? 1.0 : (
        1.0 / totalNumReadableBytes
    );
    arrayMul(invTotal, outputReadableHistogram, readableHistogramSize);
    outputOffset += readableHistogramSize;

    // :: entropy
    output[outputOffset] = calculateShannonEntropy(totalNumReadableBytes, readableHistogram, readableHistogramSize);
    outputOffset += 1;

    // :: string_counts
    // counts of interesting strings
    size_t const* const interestingStringHistogram = interestingStringCounter.getByteCountsArray();
    size_t const interestingStringHistogramSize = NUM_INTERESTING_STRING_REGEXES;
    arrayCopy(output + outputOffset, interestingStringHistogram, interestingStringHistogramSize);
    // outputOffset += interestingStringHistogramSize;
}

size_t StringExtractor::getMaxDim() const {
    return 3 + 96 + 1 + NUM_INTERESTING_STRING_REGEXES;
}
