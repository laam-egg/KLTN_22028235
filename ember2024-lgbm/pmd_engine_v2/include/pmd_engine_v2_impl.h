#pragma once

#include <stddef.h>
#include <stdint.h>

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

/** See pmd_engine_v2.h for documentation. */
#ifndef CALLEE_MANAGED
#define CALLEE_MANAGED
#endif

typedef struct _PMD2Impl_Engine_Decision {
    double score;
} PMD2Impl_Engine_Decision;

void* PMD2Impl_Engine_Init(void);

int PMD2Impl_Engine_GetNumFeatures(IN void* engine, OUT size_t* numFeatures);

/** The feature vector will be invalidated the next extraction/prediction. */
int PMD2Impl_Engine_ExtractFeatures32FromBuffer(
    IN void* engine,
    IN uint8_t const* buffer,
    IN size_t const bufferSize,
    OUT float** featureVectorPtr CALLEE_MANAGED,
    OUT size_t* featureVectorSize
);

/** The feature vector will be invalidated the next extraction/prediction. */
int PMD2Impl_Engine_ExtractFeatures32FromFilePathW(
    IN void* engine,
    IN wchar_t const* filePath,
    OUT float** featureVectorPtr CALLEE_MANAGED,
    OUT size_t* featureVectorSize
);

int PMD2Impl_Engine_PredictFromFeatures32(
    IN void* engine,
    IN float const* featureVector,
    IN size_t const featureVectorSize,
    OUT PMD2Impl_Engine_Decision* decision
);

int PMD2Impl_Engine_PredictFromBuffer(
    IN void* engine,
    IN uint8_t const* buffer,
    IN size_t const bufferSize,
    OUT PMD2Impl_Engine_Decision* decision
);

int PMD2Impl_Engine_PredictFromFilePathW(
    IN void* engine,
    IN wchar_t const* filePath,
    OUT PMD2Impl_Engine_Decision* decision
);

void PMD2Impl_Engine_Destroy(IN void* engine);
