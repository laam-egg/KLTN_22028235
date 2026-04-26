#include "efe/pefile/section.h"
#include "efe/common/bytecounter.h"
#include "efe/common/shannonentropycalculator.h"
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

std::vector<PESection> PESection::listFromPEFile(LIEF::PE::Binary const& pe, size_t const fileSize) {
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
        ss.entropy  = s.entropy(); // or compute with calculateShannonEntropy like below;
                                   // it seems both approaches are equivalent!
        
        // {
        //     size_t SIZE_TO_COMPUTE = s.content().size();
        //     // size_t SIZE_TO_COMPUTE = std::min(static_cast<size_t>(ss.vsize), s.content().size());
        //     ByteCounter byteCounter;
        //     byteCounter.reset();
        //     byteCounter.start();
        //     byteCounter.reduce(
        //         reinterpret_cast<uint8_t const*>(s.content().data()),
        //         SIZE_TO_COMPUTE
        //     );
        //     byteCounter.finalize();

        //     std::vector<size_t> counts(256);
        //     for (size_t i = 0; i < 256; ++i) {
        //         counts[i] = byteCounter.getByteCountsArray()[i];
        //     }

        //     ss.entropy = calculateShannonEntropy(
        //         SIZE_TO_COMPUTE,
        //         counts.data(),
        //         counts.size()
        //     );
        // }
        
        ss.sizeRatio  = fileSize ? double(ss.sizeRaw) / double(fileSize) : 0.0;
        ss.vsizeRatio = ss.vsize ? double(ss.sizeRaw) / double(std::max<uint32_t>(ss.vsize, 1)) : 0.0;

        ss.characteristics = s.characteristics();
        
        out.push_back(std::move(ss));

        #ifdef DEBUG
        for (size_t i = 0; i < std::min<size_t>(s.content().size(), 20); ++i) {
            printf("    char[%zu] = 0x%02x ('%c')\n", i, static_cast<unsigned int>(static_cast<uint8_t>(s.content()[i])), std::isprint(s.content()[i]) ? s.content()[i] : '.');
        }
        printf("    ...\n");
        for (int i = static_cast<int>(s.content().size()) - 20; i < static_cast<int>(s.content().size()); ++i) {
            if (i < 0) continue;
            printf("    char[%d] = 0x%02x ('%c')\n", i, static_cast<unsigned int>(static_cast<uint8_t>(s.content()[i])), std::isprint(s.content()[i]) ? s.content()[i] : '.');
        }
        #endif
    }
    return out;
}
