#ifndef EFE_PEFILE_INCLUDED
#define EFE_PEFILE_INCLUDED

// https://lief.re/doc/latest/formats/pe/index.html
#include <LIEF/PE.hpp>
#include <string>
#include <memory>
#include "efe/pefile/section.h"
#include "efe/pefile/import_library.h"
class PEFile {
private:
    std::unique_ptr<LIEF::PE::Binary> pe;
    size_t fileSize;
    std::vector<PESection> sections;
    std::vector<ImportLibrary> imports;
    bool m_isPEFile;

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
};

#endif // EFE_PEFILE_INCLUDED
