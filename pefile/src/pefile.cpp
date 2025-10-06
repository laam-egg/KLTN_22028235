#include "efe/pefile.h"
#include "efe/common/nop.h"

PEFile::PEFile(uint8_t const* const buf, size_t bufSize) {
    fileSize = bufSize;
    pe = nullptr;
    try {
        pe = LIEF::PE::Parser::parse(buf, bufSize);
    } catch (std::exception const& e) {
        pe = nullptr;
    }

    if (pe != nullptr) {
        sections = getSectionsFromPEFile(*pe, fileSize);
    }
}

bool PEFile::isPEFile() const {
    return pe != nullptr;
}

#define PE_DEFAULT(returnValue) if (!pe) { return (returnValue); }; nop()
#define IF_NO_PE if (!pe)

uint64_t PEFile::getEntrypointRVA() const {
    PE_DEFAULT(0);
    return pe->entrypoint();
}

std::vector<PESection> const& PEFile::getSections() const {
    PE_DEFAULT(sections);
    return sections;
}

void PEFile::getOverlayBytes(uint8_t const** pBuf, size_t* pBufSize) const {
    IF_NO_PE {
        pBuf[0] = NULL;
        pBufSize[0] = 0;
        return;
    }
    auto span = pe->overlay();
    pBuf[0] = span.data();
    pBufSize[0] = span.size_bytes();
}

size_t PEFile::getOverlayOffset() const {
    PE_DEFAULT(0);
    return pe->overlay_offset();
}
