#include "efe/pefile/export_library.h"
#include <cctype>
#include <string>
#include <algorithm>

std::vector<ExportFunction> ExportFunction::listFromPEFile(LIEF::PE::Binary const& pe, size_t const fileSize) {
    (void)fileSize;

    std::vector<ExportFunction> exportedFunctions;

    LIEF::PE::Export const* peExport = pe.get_export();
    if (peExport != nullptr) {
        auto const& peExportedFunctions = peExport->entries();
        exportedFunctions.reserve(peExportedFunctions.size());

        for (auto const& entry : peExportedFunctions) {
            ExportFunction f;
            f.name = entry.name();
            f.ordinal = entry.ordinal();
            exportedFunctions.emplace_back(std::move(f));
        }
    }

    return exportedFunctions;
}
