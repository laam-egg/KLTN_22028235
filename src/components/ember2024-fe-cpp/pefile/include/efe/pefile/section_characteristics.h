#ifndef PEFILE_SECTION_CHARACTERISTICS_INCLUDED
#define PEFILE_SECTION_CHARACTERISTICS_INCLUDED

#include <cstdint>
#include <vector>

using characteristic_t = uint32_t;

class SectionCharacteristic {
public:
    #define ADD(value, name) \
        static constexpr characteristic_t const name = value; \
    
    #include "section_characteristics_declarations.h"
    #undef ADD

    static std::vector<char const*> getNamesByValue(characteristic_t value);
};

#endif // PEFILE_SECTION_CHARACTERISTICS_INCLUDED
