#include "platform/ResourcePaths.h"
#include <SDL3/SDL.h>
#include <stdexcept>

namespace {
std::filesystem::path locateResources() {
#ifdef __ANDROID__
    return std::filesystem::path("."); // SDL IO reads APK assets via Android's AssetManager.
#endif
    if (const char* base = SDL_GetBasePath()) {
        const auto directory = std::filesystem::u8path(base);
        for (const auto& candidate : {directory, directory / "runtime", directory / "Resources"}) {
            if (std::filesystem::is_directory(candidate / "shaders")) return candidate;
        }
    }
    // GNU Make's test executables live in build/tests, with assets in the source tree.
    const auto current = std::filesystem::current_path();
    if (std::filesystem::is_directory(current / "shaders")) return current;
    throw std::runtime_error("Resources unavailable: expected shaders beside the application");
}

std::filesystem::path& resourceRoot() {
    static std::filesystem::path directory;
    return directory;
}
}

const std::filesystem::path& ResourcePaths::root() {
    auto& directory = resourceRoot();
    if (directory.empty()) directory = locateResources();
    return directory;
}

std::filesystem::path ResourcePaths::resolve(const std::string& relative) {
    const auto path = std::filesystem::u8path(relative);
    return (path.is_absolute() ? path : root() / path).lexically_normal();
}

void ResourcePaths::setRoot(const std::filesystem::path& directory) {
    resourceRoot() = std::filesystem::absolute(directory);
}

std::vector<unsigned char> ResourcePaths::read(const std::string& relative) {
    const auto filename = resolve(relative).u8string();
    std::size_t length = 0;
    auto* data = static_cast<unsigned char*>(SDL_LoadFile(filename.c_str(), &length));
    if (!data) return {};
    std::vector<unsigned char> bytes(data, data + length);
    SDL_free(data);
    return bytes;
}
