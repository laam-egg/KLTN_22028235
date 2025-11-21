#ifndef PESample_INCLUDED
#define PESample_INCLUDED

#include <string>
#include <vector>
#include "efe/core.h"

struct PESample {
    std::string sha256;
    std::vector<feature_t> featureVector;
    std::vector<uint8_t> fileContent;
};

#endif // PESample_INCLUDED
