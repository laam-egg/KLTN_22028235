#ifndef PEFILE_DATA_DIRECTORY_INCLUDED
#define PEFILE_DATA_DIRECTORY_INCLUDED

#include <cstdint>
#include <string>
#include <vector>
#include <LIEF/PE.hpp>

namespace DataDirectoryName {
    extern "C" typedef enum DataDirectoryName : uint8_t {
        #define ADD(name) name,

        ADD(EXPORT)
        ADD(IMPORT)
        ADD(RESOURCE)
        ADD(EXCEPTION)
        ADD(SECURITY)
        ADD(BASERELOC)
        ADD(DEBUG)
        ADD(COPYRIGHT)
        ADD(GLOBALPTR)
        ADD(TLS)
        ADD(LOAD_CONFIG)
        ADD(BOUND_IMPORT)
        ADD(IAT)
        ADD(DELAY_IMPORT)
        ADD(COM_DESCRIPTOR)
        ADD(RESERVED)
        ADD(NUM_DIRECTORY_NAMES)

        #undef ADD
    } DataDirectoryName;
}

class DataDirectory {
public:
    DataDirectoryName::DataDirectoryName index;
    uint32_t virtualAddress;
    uint32_t size;

    static std::vector<DataDirectory> listFromPEFile(LIEF::PE::Binary const& pe, size_t const fileSize);
};

#endif // PEFILE_DATA_DIRECTORY_INCLUDED
