#include "efe/core/f_header_file_info/coff_characteristics_encoding.h"
#include <cstring>

void f_header_file_info::encodeCOFFCharacteristics(coff_characteristic_t chr, feature_t* output) {
    std::memset(output, 0, COFF_CHARACTERISTICS_ENCODING_DIM * sizeof(feature_t));

    #define ADD(index, name) if (chr & COFFCharacteristic::IMAGE_FILE_##name) output[index] = 1.0;

    ADD(0, RELOCS_STRIPPED)
    ADD(1, EXECUTABLE_IMAGE)
    ADD(2, LINE_NUMS_STRIPPED)
    ADD(3, LOCAL_SYMS_STRIPPED)
    ADD(4, AGGRESIVE_WS_TRIM)
    ADD(5, LARGE_ADDRESS_AWARE)
    ADD(6, 16BIT_MACHINE)
    ADD(7, BYTES_REVERSED_LO)
    ADD(8, 32BIT_MACHINE)
    ADD(9, DEBUG_STRIPPED)
    ADD(10, REMOVABLE_RUN_FROM_SWAP)
    ADD(11, NET_RUN_FROM_SWAP)
    ADD(12, SYSTEM)
    ADD(13, DLL)
    ADD(14, UP_SYSTEM_ONLY)
    ADD(15, BYTES_REVERSED_HI)
    static_assert(15 + 1 == COFF_CHARACTERISTICS_ENCODING_DIM);

    #undef ADD
}
