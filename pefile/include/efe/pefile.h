#ifndef EFE_PEFILE_INCLUDED
#define EFE_PEFILE_INCLUDED

// https://lief.re/doc/latest/formats/pe/index.html
#include <LIEF/PE.hpp>
#include <string>
#include <memory>

class PEFile {
private:
    std::unique_ptr<LIEF::PE::Binary> pe;
    bool m_isPEFile;

public:
    PEFile(uint8_t const* const buf, size_t bufSize);
    bool isPEFile() const;
    uint64_t entrypointRVA() const;
};

#endif // EFE_PEFILE_INCLUDED
