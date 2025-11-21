#include "pe_sample_reader/pe_sample_reader.h"
#include "efe/core/common.h" // for NUM_FEATURES
#include <stdexcept>
#include <fstream>
#include <filesystem>

void openAndDecodeFileContent(
    std::vector<uint8_t>& outFileContent,
    uint8_t const* key,
    char const* hash
);
void readFileToBuf(std::vector<uint8_t>& buf, const std::string &path);
void xorDecryptBufInplace(std::vector<uint8_t>& buf, uint8_t const* key);

#define COL_HASH 0
#define COL_KEY 1
#define COL_FEATURE_VECTOR 2
#define KEY_LENGTH 32
static thread_local std::filesystem::path const TEST_BINARIES_DIR =
    std::filesystem::absolute(TEST_DATA_DIR "/resources/efe_test_data");

inline constexpr size_t FEATURE_VECTOR_BYTE_COUNT = NUM_FEATURES * sizeof(feature_t);

PESampleReader::PESampleReader(sqlite3* db) : db{db} {
    this->readStmt = NULL;

    if (this->db == NULL) {
        throw std::runtime_error("SQLite3 db is NULL???");
    }

    char const* readSql = "SELECT hash, key, feature_vector FROM files;";

    if (
        sqlite3_prepare_v2(
            this->db, readSql, -1, &this->readStmt, NULL
        ) != SQLITE_OK
    ) {
        throw std::runtime_error(
            std::string("SQLite3 statement preparation failed: ")
            + sqlite3_errmsg(db)
        );
    }
}

PESampleReader::~PESampleReader() {
    if (readStmt != NULL) {
        sqlite3_finalize(this->readStmt);
    }
}

bool PESampleReader::next(PESample& sample) {
    sqlite3_stmt* const stmt = this->readStmt;

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        return false;
    }

    unsigned char const* rawHash = sqlite3_column_text(stmt, COL_HASH);
    char const* hash = reinterpret_cast<char const*>(rawHash);
    sample.sha256 = hash;

    void const* rawKey = sqlite3_column_blob(stmt, COL_KEY);
    int rawKeyByteCount = sqlite3_column_bytes(stmt, COL_KEY);
    // SELECT DISTINCT LENGTH(key) FROM files;
    if (!rawKey || rawKeyByteCount != KEY_LENGTH) {
        throw std::runtime_error(
            "key has invalid length " + std::to_string(rawKeyByteCount)
            + "(expected " + std::to_string(KEY_LENGTH) + ") (bytes)"
        );
    }
    uint8_t const* key = reinterpret_cast<uint8_t const*>(rawKey);
    openAndDecodeFileContent(sample.fileContent, key, hash);

    void const* rawFeatureVector = sqlite3_column_blob(stmt, COL_FEATURE_VECTOR);
    int rawFeatureVectorByteCount = sqlite3_column_bytes(stmt, COL_FEATURE_VECTOR);
    // SELECT DISTINCT LENGTH(feature_vector) FROM files;
    if (!rawFeatureVector || rawFeatureVectorByteCount != FEATURE_VECTOR_BYTE_COUNT) {
        throw std::runtime_error(
            "feature_vector has invalid length " + std::to_string(rawFeatureVectorByteCount)
            + "(expected " +
                std::to_string(FEATURE_VECTOR_BYTE_COUNT)
            + ") (bytes)"
        );
    }

    sample.featureVector.reserve(NUM_FEATURES);
    feature_t const* featureVector = reinterpret_cast<feature_t const*>(rawFeatureVector);
    sample.featureVector.assign(featureVector, featureVector + NUM_FEATURES);

    return true;
}

void openAndDecodeFileContent(
    std::vector<uint8_t>& outFileContent,
    uint8_t const* key,
    char const* hash
) {
    std::filesystem::path filePath = TEST_BINARIES_DIR / std::string_view(hash);
    readFileToBuf(outFileContent, filePath);
    xorDecryptBufInplace(outFileContent, key);
}

void readFileToBuf(std::vector<uint8_t>& buf, const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Failed to open file: " + path);

    buf.assign(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>()
    );
}

/**
 * same as encryption
 */
void xorDecryptBufInplace(std::vector<uint8_t>& buf, uint8_t const* key) {
    for (size_t i = 0; i < buf.size(); ++i) {
        buf[i] = buf[i] ^ key[i % KEY_LENGTH];
    }
}
