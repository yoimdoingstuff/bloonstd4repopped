#pragma once

#include <string>
#include <vector>

namespace btd4::builder {

// Opens the host-native file picker when available. The returned paths are
// absolute paths. An empty vector means the user cancelled or no native picker
// is available on the current host.
std::vector<std::string> browseSourceFiles();

} // namespace btd4::builder
