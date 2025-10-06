#include "efe/core/f_header_file_info/subsystem_type_encoding.h"

void f_header_file_info::encodeSubsystemType(subsystem_type_t subsystemType, feature_t* output) {
    output[0] = 0.0;

    switch (subsystemType) {
        #define ADD(ordinal, name) case Subsystem::name: output[0] = ordinal; break;

        ADD(0, IMAGE_SUBSYSTEM_UNKNOWN)
        ADD(1, IMAGE_SUBSYSTEM_NATIVE)
        ADD(2, IMAGE_SUBSYSTEM_WINDOWS_GUI)
        ADD(3, IMAGE_SUBSYSTEM_WINDOWS_CUI)
        ADD(4, IMAGE_SUBSYSTEM_OS2_CUI)
        ADD(5, IMAGE_SUBSYSTEM_POSIX_CUI)
        ADD(6, IMAGE_SUBSYSTEM_NATIVE_WINDOWS)
        ADD(7, IMAGE_SUBSYSTEM_WINDOWS_CE_GUI)
        ADD(8, IMAGE_SUBSYSTEM_EFI_APPLICATION)
        ADD(9, IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER)
        ADD(10, IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER)
        ADD(11, IMAGE_SUBSYSTEM_EFI_ROM)
        ADD(12, IMAGE_SUBSYSTEM_XBOX)
        ADD(13, IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION)
        static_assert(1 == SUBSYSTEM_TYPE_ENCODING_DIM);

        #undef ADD
    }
}
