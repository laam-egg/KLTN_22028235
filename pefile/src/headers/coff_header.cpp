#include "efe/pefile/headers/coff_header.h"

CoffHeader CoffHeader::fromPEFile(LIEF::PE::Binary const& pe, size_t const fileSize) {
    // TODO: Load the struct directly from the data buffer - is that possible?
    CoffHeader coff;

    LIEF::PE::Header const& peCoff = pe.header();

    coff.Machine = static_cast<machine_type_t>(peCoff.machine());
    coff.NumberOfSections = peCoff.numberof_sections();
    coff.TimeDateStamp = peCoff.time_date_stamp();
    coff.PointerToSymbolTable = peCoff.pointerto_symbol_table();
    coff.NumberOfSymbols = peCoff.numberof_symbols();
    coff.SizeOfOptionalHeader = peCoff.sizeof_optional_header();
    coff.Characteristics = peCoff.characteristics();

    return coff;
}
