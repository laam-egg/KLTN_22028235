#ifndef EFE_HEADERS_SUBSYSTEM_TYPE_ENCODING_INCLUDED
#define EFE_HEADERS_SUBSYSTEM_TYPE_ENCODING_INCLUDED

#include "efe/pefile/subsystem_types.h"
#include "efe/featurehasher.h"

#define SUBSYSTEM_TYPE_ENCODING_DIM 1

namespace f_header_file_info {
    void encodeSubsystemType(subsystem_type_t subsystemType, feature_t* output);
}

#endif // EFE_HEADERS_SUBSYSTEM_TYPE_ENCODING_INCLUDED
