#include "efe/pefile.h"
#include "efe/common/nop.h"

PEFile::PEFile(uint8_t const* const fileContent, size_t bufSize) {
    fileSize = bufSize;
    pe = nullptr;
    try {
        pe = LIEF::PE::Parser::parse(fileContent, bufSize);
    } catch (std::exception const& e) {
        pe = nullptr;
    }

    if (pe != nullptr) {
        sections = PESection::listFromPEFile(*pe, fileSize);
        imports = ImportLibrary::listFromPEFile(*pe, fileSize);
        exportedFunctions = ExportFunction::listFromPEFile(*pe, fileSize);

        dosHeader = DOSHeader::fromPEFile(fileContent, fileSize);
        coffHeader = CoffHeader::fromPEFile(*pe, fileSize);
        optionalHeader = OptionalHeader::fromPEFile(*pe, fileSize);

        dataDirectories = DataDirectory::listFromPEFile(*pe, fileSize);
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

bool PEFile::hasImportDirectory() const {
    PE_DEFAULT(false);
    return !imports.empty();
}

std::vector<ImportLibrary> const& PEFile::getImportLibraries() const {
    PE_DEFAULT(imports);
    return imports;
}

bool PEFile::hasExportDirectory() const {
    PE_DEFAULT(false);
    return !exportedFunctions.empty();
}

std::vector<ExportFunction> const& PEFile::getExportedFunctions() const {
    PE_DEFAULT(exportedFunctions);
    return exportedFunctions;
}

bool PEFile::hasRelocs() const {
    PE_DEFAULT(false);
    return pe->has_relocations();
}

bool PEFile::hasDynamicRelocs() const {
    PE_DEFAULT(false);
    if (!pe->has_configuration()) {
        return false;
    }
    LIEF::PE::LoadConfiguration const* lc = pe->load_configuration();
    if (lc == NULL) {
        return false;
    }

    return !lc->dynamic_relocations().empty();
}
