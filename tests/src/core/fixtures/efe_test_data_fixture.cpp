#include "testing/core/fixtures/efe_test_data_fixture.h"

#include <string>

sqlite3* EFEFixtureTest::db = NULL;

void EFEFixtureTest::SetUpTestSuite() {
    // Runs ONCE for this TEST_F suite

    char const* dbPath = TEST_DATA_DIR "/resources/efe_test_data/index.sqlite3";

    int rc = sqlite3_open_v2(dbPath, &db, SQLITE_OPEN_READONLY, NULL);

    if (rc != SQLITE_OK) {
        FAIL() << "Cannot open database in read-only mode: " << sqlite3_errmsg(db);
    }
}

void EFEFixtureTest::TearDownTestSuite() {
    // Runs ONCE for this TEST_F suite
    if (db) {
        sqlite3_close(db);
    }
}

void EFEFixtureTest::SetUp() {
    // Code to run before each test in this fixture
}

void EFEFixtureTest::TearDown() {
    // Code to run after each test in this fixture
}

PESampleReader EFEFixtureTest::createPESampleReader() {
    return PESampleReader(db);
}
