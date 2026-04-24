#ifndef EMBER2024LGBM_DIRECT_MODEL_INCLUDED
#define EMBER2024LGBM_DIRECT_MODEL_INCLUDED

#include <exception>
#include <filesystem>

void* loadModel(const char* lgbmModelFileContent, int& numFeatures);
double runInference(void* _booster, feature_t const* featureVector, size_t dim);
void unloadModel(void* booster);

#endif // EMBER2024LGBM_DIRECT_MODEL_INCLUDED
