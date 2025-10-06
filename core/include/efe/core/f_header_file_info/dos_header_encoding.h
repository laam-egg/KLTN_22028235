#ifndef EFE_DOS_HEADER_ENCODING_INCLUDED
#define EFE_DOS_HEADER_ENCODING_INCLUDED

#include "efe/pefile/headers/dos_header.h"
#include "efe/featurehasher.h"

#define DOS_HEADERS_ENCODING_DIM 17

namespace f_header_file_info {
    void encodeDOSHeaders(DOSHeader const& dosHeader, feature_t* output);
}

#endif // EFE_DOS_HEADER_ENCODING_INCLUDED
