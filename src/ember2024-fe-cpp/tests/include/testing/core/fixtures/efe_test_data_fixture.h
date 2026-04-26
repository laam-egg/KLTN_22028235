#include <gtest/gtest.h>
#include <sqlite3.h>
#include <filesystem>
#include "pe_sample_reader/pe_sample_reader.h"

class EFEFixtureTest : public ::testing::Test {
protected:
    static sqlite3* db;
    
    static void SetUpTestSuite();

    static void TearDownTestSuite();

    void SetUp() override;
    void TearDown() override;

    PESampleReader createPESampleReader();
};
