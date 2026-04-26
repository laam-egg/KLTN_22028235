#ifndef PEFILE_OPTIONAL_HEADER_INCLUDED
#define PEFILE_OPTIONAL_HEADER_INCLUDED

#include <cstdint>
#include <LIEF/PE.hpp>
#include "efe/pefile/dll_characteristics.h"

struct OptionalHeader {
    uint16_t Magic = 0;
    uint8_t MajorLinkerVersion = 0;
    uint8_t MinorLinkerVersion = 0;
    uint32_t SizeOfCode = 0;
    uint32_t SizeOfInitializedData = 0;
    uint32_t SizeOfUninitializedData = 0;
    uint32_t AddressOfEntryPoint = 0;
    uint32_t BaseOfCode = 0;
    uint64_t ImageBase = 0;
    uint32_t SectionAlignment = 0;
    uint32_t SizeOfImage = 0;
    uint32_t SizeOfHeaders = 0;
    uint32_t CheckSum = 0;
    uint16_t Subsystem = 0;
    dll_characteristic_t DllCharacteristics = 0;
    uint16_t MajorOperatingSystemVersion = 0;
    uint16_t MinorOperatingSystemVersion = 0;
    uint16_t MajorImageVersion = 0;
    uint16_t MinorImageVersion = 0;
    uint16_t MajorSubsystemVersion = 0;
    uint16_t MinorSubsystemVersion = 0;
    uint32_t SizeOfStackReserve = 0;
    uint32_t SizeOfStackCommit = 0;
    uint32_t SizeOfHeapReserve = 0;
    uint32_t SizeOfHeapCommit = 0;
    uint32_t NumberOfRvaAndSizes = 0;

    static OptionalHeader fromPEFile(LIEF::PE::Binary const& pe, size_t const fileSize);
};


#endif // PEFILE_OPTIONAL_HEADER_INCLUDED
