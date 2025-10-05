#include "efe/pefile.h"

PEFile::PEFile(std::string filePath) {
    pe = LIEF::PE::Parser::parse(filePath);
    m_isPEFile = true; // TODO
}
