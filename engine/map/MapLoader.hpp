#pragma once
#include "Map.hpp"
#include "core/IFileSystem.hpp"
#include <string_view>

namespace btd4 {
// Transactional: output is unchanged on failure; error contains a diagnostic.
// Only the versioned internal map schema is accepted, never source game formats.
bool parseMap(std::string_view json, Map& output, std::string& error);
bool loadMap(const IFileSystem& files, const std::string& path,
             Map& output, std::string& error);
}
