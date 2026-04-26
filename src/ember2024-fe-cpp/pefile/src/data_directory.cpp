#include "efe/pefile/data_directory.h"

std::vector<DataDirectory> DataDirectory::listFromPEFile(LIEF::PE::Binary const& pe, size_t const fileSize) {
    (void)fileSize;

    std::vector<DataDirectory> dds;

    auto const& peDDs = pe.data_directories();
    size_t const N = std::min(peDDs.size(), static_cast<size_t>(DataDirectoryName::NUM_DIRECTORY_NAMES));
    dds.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        auto const& peDD = peDDs[i];
        dds.push_back({
            static_cast<DataDirectoryName::DataDirectoryName>(i),
            peDD.RVA(),
            peDD.size()
        });
    }

    return dds;
}
