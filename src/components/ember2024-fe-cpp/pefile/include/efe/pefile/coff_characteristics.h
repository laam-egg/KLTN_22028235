#ifndef PEFILE_COFF_CHARACTERISTICS_INCLUDED
#define PEFILE_COFF_CHARACTERISTICS_INCLUDED

#include <cstdint>

using coff_characteristic_t = uint16_t;

class COFFCharacteristic {
public: 
    #define ADD(value, name) \
        static constexpr coff_characteristic_t const name = value; \
    
    #include "coff_characteristics_declarations.h"

    #undef ADD
};

#endif // PEFILE_COFF_CHARACTERISTICS_INCLUDED
