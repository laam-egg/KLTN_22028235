#ifndef EFE_HEADERS_DLL_CHARACTERISTICS_ENCODING_INCLUDED
#define EFE_HEADERS_DLL_CHARACTERISTICS_ENCODING_INCLUDED

#include "efe/pefile/dll_characteristics.h"
#include "efe/featurehasher.h"

#define DLL_CHARACTERISTICS_ENCODING_DIM 11

namespace f_header_file_info {
    void encodeDLLCharacteristics(dll_characteristic_t chr, feature_t* output);
}

#endif // EFE_HEADERS_DLL_CHARACTERISTICS_ENCODING_INCLUDED
