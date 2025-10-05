#include "efe/pefile.h"
#include "efe/common/nop.h"

PEFile::PEFile(uint8_t const* const buf, size_t bufSize) {
    pe = nullptr;
    try {
        pe = LIEF::PE::Parser::parse(buf, bufSize);
    } catch (std::exception const& e) {
        pe = nullptr;
    }
}

bool PEFile::isPEFile() const {
    return pe != nullptr;
}

#define PE_DEFAULT(returnValue) if (!pe) { return (returnValue); }; nop()

uint64_t PEFile::entrypointRVA() const {
    PE_DEFAULT(0);
    return pe->entrypoint();
}


