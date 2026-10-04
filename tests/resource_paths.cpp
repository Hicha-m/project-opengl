#include "platform/ResourcePaths.h"
#include "audio/MusicPlayer.h"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>

int main() {
    const auto root = ResourcePaths::root();
    const auto originalWorkingDirectory = std::filesystem::current_path();
    const auto temporary = std::filesystem::temp_directory_path() /
        std::filesystem::u8path("space-étoiles-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(temporary / "build/music");
    std::filesystem::current_path(temporary);
    // Starting from an unrelated directory still finds executable-relative assets.
    assert(ResourcePaths::root() == root);
    assert(std::ifstream(ResourcePaths::resolve("shaders/earth.vert")).good());
    {
        MusicPlayer music;
        assert(music.load("build/music/cinematic.wav"));
        assert(music.loadImpact("build/music/impact.wav"));
    }
    for (const auto* filename : {"cinematic.wav", "impact.wav"})
        std::filesystem::copy_file(root / "build/music" / filename,
                                   temporary / "build/music" / filename);
    ResourcePaths::setRoot(temporary);
    {
        MusicPlayer music;
        assert(music.load("build/music/cinematic.wav"));
        assert(music.loadImpact("build/music/impact.wav"));
        assert(music.restart());
    }
    ResourcePaths::setRoot(root);
    std::filesystem::current_path(originalWorkingDirectory);
    std::filesystem::remove_all(temporary);
    std::cout << "Resources and audio work independently of CWD and with Unicode paths\n";
}
