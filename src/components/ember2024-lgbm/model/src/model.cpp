#include "ember2024_lgbm/model.h"

#include "direct_model.h"
#include "resources.h"

EMBER2024Model::EMBER2024Model() {
    char const* modelFileContent = getModel();
    int numFeatures = 0;
    model = loadModel(modelFileContent, numFeatures);
}

EMBER2024Model::~EMBER2024Model() {
    unloadModel(model);
}

double EMBER2024Model::predict(feature_t const* vector, size_t dim) {
    return runInference(model, vector, dim);
}
