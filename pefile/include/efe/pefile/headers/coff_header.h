#ifndef PEFILE_COFF_HEADER_INCLUDED
#define PEFILE_COFF_HEADER_INCLUDED

#include <cstdint>
#include <LIEF/PE.hpp>
#include "efe/pefile/machine_types.h"
#include "efe/pefile/coff_characteristics.h"

struct CoffHeader {
    machine_type_t Machine = 0;
    uint16_t NumberOfSections = 0;
    uint32_t TimeDateStamp = 0;
    uint32_t PointerToSymbolTable = 0;
    uint32_t NumberOfSymbols = 0;
    uint16_t SizeOfOptionalHeader = 0;
    coff_characteristic_t Characteristics = 0;

    static CoffHeader fromPEFile(LIEF::PE::Binary const& pe, size_t const fileSize);
};

#endif // PEFILE_COFF_HEADER_INCLUDED
