#ifndef PEFILE_EXPORT_LIBRARY_INCLUDED
#define PEFILE_EXPORT_LIBRARY_INCLUDED

#include <cstdint>
#include <string>
#include <vector>
#include <LIEF/PE.hpp>

struct ExportFunction {
    std::string name;
    uint16_t ordinal = 0;

    static std::vector<ExportFunction> listFromPEFile(LIEF::PE::Binary const& pe, size_t const fileSize);
};

#endif // PEFILE_EXPORT_LIBRARY_INCLUDED
