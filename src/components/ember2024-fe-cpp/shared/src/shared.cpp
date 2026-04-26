#include "efe/shared.h"
#include "efe/core.h"

extern "C" {
typedef struct _EFE_Engine {
    EMBER2024FeatureExtractor fe;
} EFE_Engine;

void* EFE_Init(void) {
    EFE_Engine* engine = new(std::nothrow) EFE_Engine();
    if (engine == nullptr) {
        return NULL;
    }
    return static_cast<void*>(engine);
}

void EFE_Cleanup(IN void* extractor) {
    EFE_Engine* engine = static_cast<EFE_Engine*>(extractor);
    delete engine; // null-safe
}

int EFE_GetNumFeatures(IN void* extractor, OUT int* numFeatures) {
    EFE_Engine* engine = static_cast<EFE_Engine*>(extractor);
    if (engine == nullptr || numFeatures == nullptr) {
        return EXIT_FAILURE;
    }
    *numFeatures = static_cast<int>(engine->fe.getDim());
    return EXIT_SUCCESS;
}

int EFE_ExtractFeatures32(
    IN void* extractor,
    IN uint8_t const* data,
    IN int const dataSize,
    OUT float* features,
    IN int const numFeatures
) {
    EFE_Engine* engine = static_cast<EFE_Engine*>(extractor);
    if (engine == nullptr || data == nullptr || features == nullptr) {
        return EXIT_FAILURE;
    }

    size_t dim = engine->fe.getDim();
    if (static_cast<size_t>(numFeatures) < dim) {
        return EXIT_FAILURE;
    }

    std::error_code err;
    feature_t const* output = engine->fe.run(data, static_cast<size_t>(dataSize), err);
    if (err) {
        return EXIT_FAILURE;
    }

    static_assert(std::is_same_v<feature_t, float>);
    std::memcpy(features, output, dim * sizeof(float));
    return EXIT_SUCCESS;
}

static int EFE_Private_ExtractFeatures32FromFile(
    IN void* extractor,
    IN std::filesystem::path const& filePath,
    OUT float* features,
    IN int const numFeatures
) {
    EFE_Engine* engine = static_cast<EFE_Engine*>(extractor);
    if (engine == nullptr || features == nullptr) {
        return EXIT_FAILURE;
    }

    size_t dim = engine->fe.getDim();
    if (static_cast<size_t>(numFeatures) < dim) {
        return EXIT_FAILURE;
    }

    std::error_code err;
    feature_t const* output = engine->fe.run(filePath, err);
    if (err) {
        return EXIT_FAILURE;
    }

    static_assert(std::is_same_v<feature_t, float>);
    std::memcpy(features, output, dim * sizeof(float));
    return EXIT_SUCCESS;
}

int EFE_ExtractFeatures32FromFileA(
    IN void* extractor,
    IN char const* filePath,
    OUT float* features,
    IN int const numFeatures
) {
    if (filePath == nullptr) {
        return EXIT_FAILURE;
    }
    return EFE_Private_ExtractFeatures32FromFile(
        extractor,
        std::filesystem::path(filePath),
        features,
        numFeatures
    );
}

int EFE_ExtractFeatures32FromFileW(
    IN void* extractor,
    IN wchar_t const* filePath,
    OUT float* features,
    IN int const numFeatures
) {
    if (filePath == nullptr) {
        return EXIT_FAILURE;
    }
    return EFE_Private_ExtractFeatures32FromFile(
        extractor,
        std::filesystem::path(filePath),
        features,
        numFeatures
    );
}

} // extern "C"
