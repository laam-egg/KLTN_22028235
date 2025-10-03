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
    PEFile(std::string filePath);
    inline constexpr bool isPEFile() const { return m_isPEFile; }
};

#endif // EFE_PEFILE_INCLUDED
