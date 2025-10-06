#include "efe/pefile/section.h"
#include <cctype>
#include <string>
#include <algorithm>

bool PESection::containsRVA(uint64_t rva) const {
    uint64_t end = vaddr + std::max<uint64_t>(vsize, sizeRaw);  
    return rva >= vaddr && rva < end;
}

bool PESection::hasCharacteristic(LIEF::PE::Section::CHARACTERISTICS chr) const {
    return hasCharacteristic(static_cast<characteristic_t>(chr));
}

bool PESection::hasCharacteristic(characteristic_t chr) const {
    return (characteristics & chr) != 0;
}

std::vector<PESection> getSectionsFromPEFile(LIEF::PE::Binary const& pe, size_t const fileSize) {
    std::vector<PESection> out;
    for (auto const& s : pe.sections()) {
        PESection ss;
        std::string name = s.name();
        // trim nulls + tolower:
        name.erase(std::find(name.begin(), name.end(), '\0'), name.end());
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);

        ss.name = name;
        ss.sizeRaw = s.sizeof_raw_data();
        ss.vsize    = s.virtual_size();
        ss.vaddr    = s.virtual_address();
        ss.entropy  = s.entropy(); // or compute with calculateShannonEntropy, args (s.content().data(), s.content().size());
                                   // it seems both approaches are equivalent
        ss.sizeRatio  = fileSize ? double(ss.sizeRaw) / double(fileSize) : 0.0;
        ss.vsizeRatio = ss.vsize ? double(ss.sizeRaw) / double(std::max<uint32_t>(ss.vsize, 1)) : 0.0;

        ss.characteristics = s.characteristics();
        
        out.push_back(std::move(ss));
    }
    return out;
}
