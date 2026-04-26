#ifndef EFE_HEADERS_COFF_CHARACTERISTICS_ENCODING_INCLUDED
#define EFE_HEADERS_COFF_CHARACTERISTICS_ENCODING_INCLUDED

#include "efe/pefile/coff_characteristics.h"
#include "efe/featurehasher.h"

#define COFF_CHARACTERISTICS_ENCODING_DIM 16

namespace f_header_file_info {
    void encodeCOFFCharacteristics(coff_characteristic_t chr, feature_t* output);
}

#endif // EFE_HEADERS_COFF_CHARACTERISTICS_ENCODING_INCLUDED
