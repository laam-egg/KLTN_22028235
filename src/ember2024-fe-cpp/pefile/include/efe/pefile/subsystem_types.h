#ifndef PEFILE_SUBSYSTEM_TYPES_INCLUDED
#define PEFILE_SUBSYSTEM_TYPES_INCLUDED

#include <cstdint>

using subsystem_type_t = uint16_t;

class Subsystem {
public:
    #define ADD(value, name) \
        static constexpr subsystem_type_t const name = value; \
    
    #include "subsystem_types_declarations.h"

    #undef ADD
};

#endif // PEFILE_SUBSYSTEM_TYPES_INCLUDED
