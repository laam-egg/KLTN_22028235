#include "efe/pefile/headers/optional_header.h"

OptionalHeader OptionalHeader::fromPEFile(LIEF::PE::Binary const& pe, size_t const fileSize) {
    // TODO: Load the struct directly from the data buffer - is that possible?
    OptionalHeader oh;

    LIEF::PE::OptionalHeader const& peOptionalHeader = pe.optional_header();

    oh.Magic = static_cast<decltype(oh.Magic)>(peOptionalHeader.magic());
    oh.MajorLinkerVersion = peOptionalHeader.major_linker_version();
    oh.MinorLinkerVersion = peOptionalHeader.minor_linker_version();
    oh.SizeOfCode = peOptionalHeader.sizeof_code();
    oh.SizeOfInitializedData = peOptionalHeader.sizeof_initialized_data();
    oh.SizeOfUninitializedData = peOptionalHeader.sizeof_uninitialized_data();
    oh.AddressOfEntryPoint = peOptionalHeader.addressof_entrypoint();
    oh.BaseOfCode = peOptionalHeader.baseof_code();
    oh.ImageBase = peOptionalHeader.imagebase();
    oh.SectionAlignment = peOptionalHeader.section_alignment();
    oh.SizeOfImage = peOptionalHeader.sizeof_image();
    oh.SizeOfHeaders = peOptionalHeader.sizeof_headers();
    oh.CheckSum = peOptionalHeader.checksum();
    oh.Subsystem = static_cast<decltype(oh.Subsystem)>(peOptionalHeader.subsystem());
    oh.DllCharacteristics = peOptionalHeader.dll_characteristics();
    oh.MajorOperatingSystemVersion = peOptionalHeader.major_operating_system_version();
    oh.MinorOperatingSystemVersion = peOptionalHeader.minor_operating_system_version();
    oh.MajorImageVersion = peOptionalHeader.major_image_version();
    oh.MinorImageVersion = peOptionalHeader.minor_image_version();
    oh.MajorSubsystemVersion = peOptionalHeader.major_subsystem_version();
    oh.MinorSubsystemVersion = peOptionalHeader.minor_subsystem_version();
    oh.SizeOfStackReserve = peOptionalHeader.sizeof_stack_reserve();
    oh.SizeOfStackCommit = peOptionalHeader.sizeof_stack_commit();
    oh.SizeOfHeapReserve = peOptionalHeader.sizeof_heap_reserve();
    oh.SizeOfHeapCommit = peOptionalHeader.sizeof_heap_commit();
    oh.NumberOfRvaAndSizes = peOptionalHeader.numberof_rva_and_size();

    return oh;
}
