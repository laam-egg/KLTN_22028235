#ifndef PEFILE_MACHINE_TYPES_INCLUDED
#define PEFILE_MACHINE_TYPES_INCLUDED

#include <cstdint>

using machine_type_t = uint16_t;

class MachineType {
public:
    #define ADD(value, name) \
        static constexpr machine_type_t const name = value; \
    
    #include "machine_types_declarations.h"

    #undef ADD

    static char const* getNameByValue(machine_type_t value);
};

#endif // PEFILE_MACHINE_TYPES_INCLUDED
