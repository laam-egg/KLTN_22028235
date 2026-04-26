#ifndef EMBER2024LGBM_MODEL_INCLUDED
#define EMBER2024LGBM_MODEL_INCLUDED

#include <cstddef>
#include <exception>
#include <efe/core.h>

class EMBER2024Model {
public:
    EMBER2024Model();
    ~EMBER2024Model();

    double predict(feature_t const* vector, size_t dim);

private:
    void* model = nullptr;
};

#endif // EMBER2024LGBM_MODEL_INCLUDED
