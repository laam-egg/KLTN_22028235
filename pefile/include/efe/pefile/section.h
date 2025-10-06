#ifndef PEFILE_SECTION_INCLUDED
#define PEFILE_SECTION_INCLUDED

#include <cstdint>
#include <string>
#include <algorithm>
#include <vector>
#include <LIEF/PE.hpp>
#include "efe/pefile/section_characteristics.h"

struct PESection {
    std::string name;
    uint32_t sizeRaw;
    uint32_t vsize;
    uint64_t vaddr;
    double entropy;
    double sizeRatio;
    double vsizeRatio;
    characteristic_t characteristics;

    bool containsRVA(uint64_t rva) const;

    bool hasCharacteristic(LIEF::PE::Section::CHARACTERISTICS chr) const;
    bool hasCharacteristic(characteristic_t chr) const;
};

std::vector<PESection> getSectionsFromPEFile(LIEF::PE::Binary const& pe, size_t const fileSize);

#endif // PEFILE_SECTION_INCLUDED
