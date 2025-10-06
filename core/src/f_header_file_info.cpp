#define uint642uint32(x) static_cast<uint32_t>((x) & 0xffffffffu)

#include "efe/core/f_header_file_info.h"
#include "efe/core/f_header_file_info/all.h"

#define DIM 74

class Offset {
public:
    #define ADD(NAME, VALUE) static constexpr size_t const NAME = VALUE;

    ADD(COFF_TIMESTAMP, 0)
    ADD(COFF_NUM_SECTIONS, COFF_TIMESTAMP + 1)
    ADD(COFF_NUM_SYMBOLS, COFF_NUM_SECTIONS + 1)
    ADD(COFF_SIZEOF_OPTIONAL_HEADER, COFF_NUM_SYMBOLS + 1)
    ADD(COFF_PTR_TO_SYMBOL_TABLE, COFF_SIZEOF_OPTIONAL_HEADER + 1)
    ADD(COFF_MACHINE_TYPE, COFF_PTR_TO_SYMBOL_TABLE + MACHINE_TYPE_ENCODING_DIM)

    ADD(OPT_SUBSYSTEM_TYPE, COFF_MACHINE_TYPE + SUBSYSTEM_TYPE_ENCODING_DIM)
    ADD(OPT_MAJOR_IMAGE_VERSION, OPT_SUBSYSTEM_TYPE + 1)
    ADD(OPT_MINOR_IMAGE_VERSION, OPT_MAJOR_IMAGE_VERSION + 1)
    ADD(OPT_MAJOR_LINKER_VERSION, OPT_MINOR_IMAGE_VERSION + 1)
    ADD(OPT_MINOR_LINKER_VERSION, OPT_MAJOR_LINKER_VERSION + 1)
    ADD(OPT_MAJOR_OS_VERSION, OPT_MINOR_LINKER_VERSION + 1)
    ADD(OPT_MINOR_OS_VERSION, OPT_MAJOR_OS_VERSION + 1)
    ADD(OPT_MAJOR_SUBSYSTEM_VERSION, OPT_MINOR_OS_VERSION + 1)
    ADD(OPT_MINOR_SUBSYSTEM_VERSION, OPT_MAJOR_SUBSYSTEM_VERSION + 1)
    ADD(OPT_SIZEOF_CODE, OPT_MINOR_SUBSYSTEM_VERSION + 1)
    ADD(OPT_SIZEOF_HEADERS, OPT_SIZEOF_CODE + 1)
    ADD(OPT_SIZEOF_IMAGE, OPT_SIZEOF_HEADERS + 1)
    ADD(OPT_SIZEOF_INITIALIZED_DATA, OPT_SIZEOF_IMAGE + 1)
    ADD(OPT_SIZEOF_UNINITIALIZED_DATA, OPT_SIZEOF_INITIALIZED_DATA + 1)
    ADD(OPT_SIZEOF_STACK_RESERVE, OPT_SIZEOF_UNINITIALIZED_DATA + 1)
    ADD(OPT_SIZEOF_STACK_COMMIT, OPT_SIZEOF_STACK_RESERVE + 1)
    ADD(OPT_SIZEOF_HEAP_RESERVE, OPT_SIZEOF_STACK_COMMIT + 1)
    ADD(OPT_SIZEOF_HEAP_COMMIT, OPT_SIZEOF_HEAP_RESERVE + 1)
    ADD(OPT_ADDRESS_OF_ENTRYPOINT, OPT_SIZEOF_HEAP_COMMIT + 1)
    ADD(OPT_BASE_OF_CODE, OPT_ADDRESS_OF_ENTRYPOINT + 1)
    ADD(OPT_IMAGE_BASE, OPT_BASE_OF_CODE + 1)
    ADD(OPT_SECTION_ALIGNMENT, OPT_IMAGE_BASE + 1)
    ADD(OPT_CHECKSUM, OPT_SECTION_ALIGNMENT + 1)
    ADD(OPT_NUM_RVAS_AND_SIZES, OPT_CHECKSUM + 1)

    ADD(COFF_CHARACTERISTIC_VECTOR, OPT_NUM_RVAS_AND_SIZES + 1)
    ADD(OPT_DLL_CHARACTERISTICS_VECTOR, COFF_CHARACTERISTIC_VECTOR + COFF_CHARACTERISTICS_ENCODING_DIM)
    ADD(DOS_VECTOR, OPT_DLL_CHARACTERISTICS_VECTOR + DLL_CHARACTERISTICS_ENCODING_DIM)

    #undef ADD

    static_assert(DOS_VECTOR + DOS_HEADERS_ENCODING_DIM == DIM);
};

char const* HeaderFileInfo::getName() const {
    return "header";
}

void HeaderFileInfo::reset(feature_t* output, PEFile const& peFile) {
    // pass
}

void HeaderFileInfo::start(feature_t* output, PEFile const& peFile) {
    std::memset(output, 0, DIM * sizeof(output[0]));

    if (!peFile.isPEFile()) {
        return;
    }

    auto const& coff = peFile.getCOFFHeader();
    output[Offset::COFF_TIMESTAMP] = coff.TimeDateStamp;
    output[Offset::COFF_NUM_SECTIONS] = coff.NumberOfSections;
    output[Offset::COFF_NUM_SYMBOLS] = coff.NumberOfSymbols;
    output[Offset::COFF_SIZEOF_OPTIONAL_HEADER] = coff.SizeOfOptionalHeader;
    output[Offset::COFF_PTR_TO_SYMBOL_TABLE] = coff.PointerToSymbolTable;
    f_header_file_info::encodeMachineType(coff.Machine, output + Offset::COFF_MACHINE_TYPE);

    auto const& opt = peFile.getOptionalHeader();
    f_header_file_info::encodeSubsystemType(opt.Subsystem, output + Offset::OPT_SUBSYSTEM_TYPE);
    output[Offset::OPT_MAJOR_IMAGE_VERSION] = opt.MajorImageVersion;
    output[Offset::OPT_MINOR_IMAGE_VERSION] = opt.MinorImageVersion;
    output[Offset::OPT_MAJOR_LINKER_VERSION] = opt.MajorLinkerVersion;
    output[Offset::OPT_MINOR_LINKER_VERSION] = opt.MinorLinkerVersion;
    output[Offset::OPT_MAJOR_OS_VERSION] = opt.MajorOperatingSystemVersion;
    output[Offset::OPT_MINOR_OS_VERSION] = opt.MinorOperatingSystemVersion;
    output[Offset::OPT_MAJOR_SUBSYSTEM_VERSION] = opt.MajorSubsystemVersion;
    output[Offset::OPT_MINOR_SUBSYSTEM_VERSION] = opt.MinorSubsystemVersion;
    output[Offset::OPT_SIZEOF_CODE] = opt.SizeOfCode;
    output[Offset::OPT_SIZEOF_HEADERS] = opt.SizeOfHeaders;
    output[Offset::OPT_SIZEOF_IMAGE] = opt.SizeOfImage;
    output[Offset::OPT_SIZEOF_INITIALIZED_DATA] = opt.SizeOfInitializedData;
    output[Offset::OPT_SIZEOF_UNINITIALIZED_DATA] = opt.SizeOfUninitializedData;
    output[Offset::OPT_SIZEOF_STACK_RESERVE] = opt.SizeOfStackReserve;
    output[Offset::OPT_SIZEOF_STACK_COMMIT] = opt.SizeOfStackCommit;
    output[Offset::OPT_SIZEOF_HEAP_RESERVE] = opt.SizeOfHeapReserve;
    output[Offset::OPT_SIZEOF_HEAP_COMMIT] = opt.SizeOfHeapCommit;
    output[Offset::OPT_ADDRESS_OF_ENTRYPOINT] = opt.AddressOfEntryPoint;
    output[Offset::OPT_BASE_OF_CODE] = opt.BaseOfCode;
    output[Offset::OPT_IMAGE_BASE] = uint642uint32(opt.ImageBase);
    output[Offset::OPT_SECTION_ALIGNMENT] = opt.SectionAlignment;
    output[Offset::OPT_CHECKSUM] = opt.CheckSum;
    output[Offset::OPT_NUM_RVAS_AND_SIZES] = opt.NumberOfRvaAndSizes;
    f_header_file_info::encodeCOFFCharacteristics(coff.Characteristics, output + Offset::COFF_CHARACTERISTIC_VECTOR);
    f_header_file_info::encodeDLLCharacteristics(opt.DllCharacteristics, output + Offset::OPT_DLL_CHARACTERISTICS_VECTOR);

    auto const& dos = peFile.getDOSHeader();
    f_header_file_info::encodeDOSHeaders(dos, output + Offset::DOS_VECTOR);
}

void HeaderFileInfo::reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize){
    // pass
}

void HeaderFileInfo::finalize(feature_t* output, PEFile const& peFile){
    // pass
}

size_t HeaderFileInfo::getMaxDim() const {
    return DIM;
}
