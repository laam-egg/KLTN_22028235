#ifndef EFE_PEFILE_INCLUDED
#define EFE_PEFILE_INCLUDED

// https://lief.re/doc/latest/formats/pe/index.html
#include <LIEF/PE.hpp>
#include <string>
#include <memory>
#include "efe/pefile/section.h"
#include "efe/pefile/import_library.h"
#include "efe/pefile/export_library.h"
#include "efe/pefile/headers/dos_header.h"
#include "efe/pefile/headers/coff_header.h"
#include "efe/pefile/headers/optional_header.h"
#include "efe/pefile/data_directory.h"

class PEFile {
private:
    std::unique_ptr<LIEF::PE::Binary> pe;
    size_t fileSize;

    std::vector<PESection> sections;
    std::vector<ImportLibrary> imports;
    std::vector<ExportFunction> exportedFunctions;

    DOSHeader dosHeader;
    CoffHeader coffHeader;
    OptionalHeader optionalHeader;

    std::vector<DataDirectory> dataDirectories;

    std::vector<uint8_t> richHeaderRaw;

public:
    PEFile(uint8_t const* const buf, size_t bufSize);
    bool isPEFile() const;
    inline constexpr size_t getFileSize() const { return fileSize; }
    uint64_t getEntrypointRVA() const;
    std::vector<PESection> const& getSections() const;
    void getOverlayBytes(uint8_t const** pBuf, size_t* pBufSize) const;
    size_t getOverlayOffset() const;

    bool hasImportDirectory() const;
    std::vector<ImportLibrary> const& getImportLibraries() const;

    bool hasExportDirectory() const;
    std::vector<ExportFunction> const& getExportedFunctions() const;

    inline constexpr DOSHeader const& getDOSHeader() const { return dosHeader; }
    inline constexpr CoffHeader const& getCOFFHeader() const { return coffHeader; }
    inline constexpr OptionalHeader const& getOptionalHeader() const { return optionalHeader; }

    bool hasRelocs() const;
    bool hasDynamicRelocs() const;
    inline constexpr std::vector<DataDirectory> const& getDataDirectories() const { return dataDirectories; }

    bool hasRichHeader() const;
    void getRichHeaderBytes(uint8_t const** pBuf, size_t* pBufSize) const;
};

#endif // EFE_PEFILE_INCLUDED
