#include "efe/core/f_data_directories.h"

#define DIM 34

#define DATA_DIRECTORIES_VECTOR_DIM 32

class Offset {
public:
    #define ADD(NAME, VALUE) static constexpr size_t const NAME = VALUE;

    ADD(DATA_DIRECTORIES_VECTOR, 0)
    ADD(HAS_RELOCS, DATA_DIRECTORIES_VECTOR + DATA_DIRECTORIES_VECTOR_DIM)
    ADD(HAS_DYNAMIC_RELOCS, HAS_RELOCS + 1)

    #undef ADD

    static_assert(HAS_DYNAMIC_RELOCS + 1 == DIM);
};

char const* DataDirectories::getName() const {
    return "datadirectories";
}

void DataDirectories::reset(feature_t* output, PEFile const& peFile) {
    // pass
}

void DataDirectories::start(feature_t* output, PEFile const& peFile) {
    std::memset(output, 0, DIM * sizeof(output[0]));

    if (!peFile.isPEFile()) {
        return;
    }

    feature_t* const ddVectorOutput = output + Offset::DATA_DIRECTORIES_VECTOR;
    for (auto const& dd : peFile.getDataDirectories()) {
        // It just so happens that the _name_order
        // in original Python code has the same indexes
        // for the corresponding values, so no conversion
        // is needed here.
        size_t const idx = dd.index;
        if (idx * 2 + 1 >= DATA_DIRECTORIES_VECTOR_DIM || idx * 2 >= DATA_DIRECTORIES_VECTOR_DIM) {
            // TODO: What to do here? Temporary ignore it.
            continue;
        }
        ddVectorOutput[2 * idx] = dd.size;
        ddVectorOutput[2 * idx + 1] = dd.virtualAddress;
    }

    output[Offset::HAS_RELOCS] = peFile.hasRelocs() ? 1.0 : 0.0;
    output[Offset::HAS_DYNAMIC_RELOCS] = peFile.hasDynamicRelocs() ? 1.0 : 0.0;
}

void DataDirectories::reduce(feature_t* output, PEFile const& peFile, size_t bufOffset, uint8_t const* buf, size_t bufSize) {
    // pass
}

void DataDirectories::finalize(feature_t* output, PEFile const& peFile) {
    // pass
}

size_t DataDirectories::getMaxDim() const {
    return DIM;
}
