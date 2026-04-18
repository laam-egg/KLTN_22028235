#ifndef ENGINE_RESOURCES_INCLUDED
#define ENGINE_RESOURCES_INCLUDED

#include <cmrc/cmrc.hpp>
#include <string>
#include <filesystem>

CMRC_DECLARE(engine_resources);

std::filesystem::path writePMDEngineDLLContent();

#endif // ENGINE_RESOURCES_INCLUDED
