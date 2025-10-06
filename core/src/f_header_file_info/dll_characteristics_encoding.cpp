#include "efe/core/f_header_file_info/dll_characteristics_encoding.h"
#include <cstring>

void f_header_file_info::encodeDLLCharacteristics(dll_characteristic_t chr, feature_t* output) {
    std::memset(output, 0, DLL_CHARACTERISTICS_ENCODING_DIM * sizeof(feature_t));

    #define ADD(index, name) if (chr & DLLCharacteristic::IMAGE_DLLCHARACTERISTICS_##name) output[index] = 1.0;
    ADD(0, HIGH_ENTROPY_VA)
    ADD(1, DYNAMIC_BASE)
    ADD(2, FORCE_INTEGRITY)
    ADD(3, NX_COMPAT)
    ADD(4, NO_ISOLATION)
    ADD(5, NO_SEH)
    ADD(6, NO_BIND)
    ADD(7, APPCONTAINER)
    ADD(8, WDM_DRIVER)
    ADD(9, GUARD_CF)
    ADD(10, TERMINAL_SERVER_AWARE)
    static_assert(10 + 1 == DLL_CHARACTERISTICS_ENCODING_DIM);
    #undef ADD
}
