#include "efe/core/f_header_file_info/dos_header_encoding.h"

void f_header_file_info::encodeDOSHeaders(DOSHeader const& dosHeader, feature_t* output) {
    #define ADD(i, FIELD) output[i] = dosHeader.fields.FIELD;

    ADD(0, e_magic)
    ADD(1, e_cblp)
    ADD(2, e_cp)
    ADD(3, e_crlc)
    ADD(4, e_cparhdr)
    ADD(5, e_minalloc)
    ADD(6, e_maxalloc)
    ADD(7, e_ss)
    ADD(8, e_sp)
    ADD(9, e_csum)
    ADD(10, e_ip)
    ADD(11, e_cs)
    ADD(12, e_lfarlc)
    ADD(13, e_ovno)
    ADD(14, e_oemid)
    ADD(15, e_oeminfo)
    ADD(16, e_lfanew)
    static_assert(16 + 1 == DOS_HEADERS_ENCODING_DIM);

    #undef ADD
}
