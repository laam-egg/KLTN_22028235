#ifndef PEFILE_IMPORT_LIBRARY_INCLUDED
#define PEFILE_IMPORT_LIBRARY_INCLUDED

#include <cstdint>
#include <string>
#include <vector>
#include <LIEF/PE.hpp>

struct ImportFunction {
    std::string name;       // May be empty if imported by ordinal
    uint16_t ordinal = 0;   // 0 if not imported by ordinal
};

struct ImportLibrary {
    std::string dllName;
    std::vector<ImportFunction> functions;

    static std::vector<ImportLibrary> listFromPEFile(LIEF::PE::Binary const& pe, size_t const fileSize);
};

#endif // PEFILE_IMPORT_LIBRARY_INCLUDED
