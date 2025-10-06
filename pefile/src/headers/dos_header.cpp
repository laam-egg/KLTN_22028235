#include "efe/pefile/headers/dos_header.h"
#include "efe/pefile/common.h"

DOSHeader DOSHeader::fromPEFile(uint8_t const* fileContent, size_t const fileSize) {
    DOSHeader dh;

    bool isPEFile = true;

    #define NOT_A_PE_FILE(reason) { isPEFile = false; goto VERDICT; }

    if (fileSize < DOS_HEADER_SIZE) {
        NOT_A_PE_FILE("Unable to read the DOS Header, possibly a truncated file.")
    }

    std::memcpy(&dh.fields, fileContent, fileSize);

    if (dh.fields.e_magic == IMAGE_DOSZM_SIGNATURE) {
        NOT_A_PE_FILE("Probably a ZM Executable (not a PE file).")
    }

    if (dh.fields.e_magic != IMAGE_DOS_SIGNATURE) {
        NOT_A_PE_FILE("DOS Header magic not found.")
    }

    if (dh.fields.e_lfanew > fileSize) {
        NOT_A_PE_FILE("Invalid e_lfanew value, probably not a PE file")
    }

    VERDICT:
    if (!isPEFile) {
        std::memset(&dh.fields, 0, DOS_HEADER_SIZE);
    }

    return dh;
}
