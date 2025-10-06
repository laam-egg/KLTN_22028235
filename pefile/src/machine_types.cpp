#include "efe/pefile/machine_types.h"

char const* MachineType::getNameByValue(machine_type_t value) {
    switch (value) {
        #define ADD(value, name) \
            case value: return #name;

        #include "efe/pefile/machine_types_declarations.h"

        #undef ADD

        default:
            // return getNameByValue(MachineType::IMAGE_FILE_MACHINE_UNKNOWN);
            return "IMAGE_FILE_MACHINE_UNKNOWN";
    }
}
