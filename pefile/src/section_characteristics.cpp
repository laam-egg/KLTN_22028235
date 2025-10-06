#include "efe/pefile/section_characteristics.h"

std::vector<char const*> SectionCharacteristic::getNamesByValue(characteristic_t sum) {
    std::vector<char const*> names;

    #define ADD(_, name) \
        if ((sum) & SectionCharacteristic::name) { \
            names.push_back(#name); \
        } \

    #include "efe/pefile/section_characteristics_declarations.h"
    #undef ADD

    return names;
}
