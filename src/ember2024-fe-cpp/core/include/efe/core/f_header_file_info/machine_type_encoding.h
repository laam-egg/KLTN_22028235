#ifndef EFE_HEADERS_MACHINE_TYPE_ENCODING_INCLUDED
#define EFE_HEADERS_MACHINE_TYPE_ENCODING_INCLUDED

#include "efe/pefile/machine_types.h"
#include "efe/featurehasher.h"

#define MACHINE_TYPE_ENCODING_DIM 1

namespace f_header_file_info {
    void encodeMachineType(machine_type_t machineType, feature_t* output);
}

#endif // EFE_HEADERS_MACHINE_TYPE_ENCODING_INCLUDED
