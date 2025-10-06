#include "efe/pefile/import_library.h"
#include <cctype>
#include <string>
#include <algorithm>

std::vector<ImportLibrary> ImportLibrary::listFromPEFile(LIEF::PE::Binary const& pe, size_t const fileSize) {
    (void)fileSize;
    
    std::vector<ImportLibrary> imports;
    imports.reserve(pe.imports().size());

    for (auto const& import : pe.imports()) {
        ImportLibrary lib;
        lib.functions.reserve(import.entries().size());

        std::string dllName = import.name();
        std::transform(dllName.begin(), dllName.end(), dllName.begin(), ::tolower);
        lib.dllName = dllName;

        for (auto const& entry : import.entries()) {
            ImportFunction f;
            if (entry.is_ordinal()) {
                f.ordinal = entry.ordinal();
            } else {
                f.name = entry.name();
            }
            lib.functions.emplace_back(std::move(f));
        }

        imports.emplace_back(std::move(lib));
    }

    return imports;
}