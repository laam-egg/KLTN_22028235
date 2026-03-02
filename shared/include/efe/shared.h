#ifndef shared
#define shared

#include "efe_shared_export.h"
#include <cstdint>

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

extern "C" {
    EFE_SHARED_EXPORT void* EFE_Init(void);

    EFE_SHARED_EXPORT int EFE_GetNumFeatures(IN void* extractor, OUT int* numFeatures);

    EFE_SHARED_EXPORT int EFE_ExtractFeatures32(
        IN void* extractor,
        IN uint8_t const* data,
        IN int const dataSize,
        OUT float* features,
        IN int const numFeatures
    );

    EFE_SHARED_EXPORT int EFE_ExtractFeatures32FromFileA(
        IN void* extractor,
        IN char const* filePath,
        OUT float* features,
        IN int const numFeatures
    );

    EFE_SHARED_EXPORT int EFE_ExtractFeatures32FromFileW(
        IN void* extractor,
        IN wchar_t const* filePath,
        OUT float* features,
        IN int const numFeatures
    );

    EFE_SHARED_EXPORT void EFE_Cleanup(IN void* extractor);
}

#endif // shared
