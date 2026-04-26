#ifndef PESampleReader_INCLUDED
#define PESampleReader_INCLUDED

#include <sqlite3.h>
#include "pe_sample_reader/pe_sample.h"

class PESampleReader {
public:
    PESampleReader(sqlite3* db);
    ~PESampleReader();
    bool next(PESample& sample);

private:
    sqlite3* const db;
    sqlite3_stmt* readStmt;
};

#endif // PESampleReader_INCLUDED
