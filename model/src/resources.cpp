#include "resources.h"

char const* getModel() {
    auto fs = cmrc::model_resources::get_filesystem();
    auto modelFile = fs.open("EMBER2024_all.model");
    return modelFile.cbegin();
}
