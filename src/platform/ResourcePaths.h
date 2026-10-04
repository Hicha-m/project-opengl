#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace ResourcePaths {
// Resources beside the executable (or inside a macOS .app), never tied to CWD.
const std::filesystem::path& root();
std::filesystem::path resolve(const std::string& relative);
// Explicit override for missing-resource tests and custom installations.
std::vector<unsigned char> read(const std::string& relative);
void setRoot(const std::filesystem::path& directory);
}
