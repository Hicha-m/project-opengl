#include "assets/PreparedAssets.h"
#include "platform/ResourcePaths.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include "stb_image.h"

int main()
{
    const auto root = ResourcePaths::root();
    const auto prepared = readShuttle(root / "build/assets/shuttle.bin");
    const auto source = readOBJ(root / "models/shuttle/shuttle.obj");
    assert(prepared.geometry.vertices.size() == 33957 && prepared.geometry.sections.size() == 13);
    glm::vec3 low = source.vertices.front().position, high = low;
    for (const auto& v : source.vertices)
    {
        low = glm::min(low, v.position);
        high = glm::max(high, v.position);
    }
    const auto center = (low + high) * 0.5f, extent = high - low;
    const float size = std::max({extent.x, extent.y, extent.z});
    for (std::size_t i = 0; i < source.vertices.size(); ++i)
    {
        const auto& a = source.vertices[i];
        const auto& b = prepared.geometry.vertices[i];
        assert(b.position == (a.position - center) / size);
        assert(a.normal == b.normal && a.texCoords == b.texCoords && a.tangent == b.tangent);
    }
    for (std::size_t i = 0; i < source.sections.size(); ++i)
    {
        const auto& a = source.sections[i];
        const auto& b = prepared.geometry.sections[i];
        assert(a.first == b.first && a.count == b.count && a.material == b.material);
    }
    const auto expected =
        prepareShuttle(root / "models/shuttle/shuttle.obj", root / "models/shuttle/shuttle.mtl");
    for (const auto& entry : expected.surfaces)
    {
        const auto& a = entry.second;
        const auto& b = prepared.surfaces.at(entry.first);
        assert(a.color == b.color && a.metallic == b.metallic && a.roughness == b.roughness);
        assert(a.diffuse == b.diffuse && a.normal == b.normal);
    }
    unsigned textures = 0;
    for (const auto& directory : {root / "textures", root / "models/shuttle"})
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
        {
            const auto ext = entry.path().extension();
            if (ext != ".png" && ext != ".jpg")
            {
                continue;
            }
            const auto relative = std::filesystem::relative(entry.path(), root);
            const auto texture = readTexture(root / "build/assets" / (relative.generic_string() + ".rgba"));
            int w, h, channels;
            std::unique_ptr<unsigned char, decltype(&stbi_image_free)> original(
                stbi_load(entry.path().u8string().c_str(), &w, &h, &channels, 4), stbi_image_free);
            assert(original && texture.width == unsigned(w) && texture.height == unsigned(h));
            const auto row = std::size_t(w) * 4;
            for (int y = 0; y < h; ++y)
            {
                assert(std::equal(original.get() + std::size_t(h - y - 1) * row,
                                  original.get() + std::size_t(h - y) * row,
                                  texture.pixels.data() + std::size_t(y) * row));
            }
            ++textures;
        }
    }
    const auto shaders = readShaders(root / "build/assets/shaders.bin");
    for (const auto& shader : shaders)
    {
        std::ifstream input(root / shader.first, std::ios::binary);
        assert(input);
        assert(shader.second == std::string(std::istreambuf_iterator<char>(input), {}));
    }
    auto temporary = std::filesystem::temp_directory_path() /
                     ("prepared-assets-test-" +
                      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(temporary);
    struct Cleanup
    {
        std::filesystem::path p;
        ~Cleanup() { std::filesystem::remove_all(p); }
    } cleanup{temporary};
    for (const std::string type : {"shuttle.bin", "shaders.bin", "textures/moon/2k_moon.jpg.rgba"})
    {
        const auto path = temporary / "broken.bin";
        auto reject = [&]()
        {
            bool failed = false;
            try
            {
                if (type == "shuttle.bin")
                {
                    readShuttle(path);
                }
                else if (type == "shaders.bin")
                {
                    readShaders(path);
                }
                else
                {
                    readTexture(path);
                }
            }
            catch (const std::exception&)
            {
                failed = true;
            }
            assert(failed);
        };
        std::filesystem::copy_file(root / "build/assets" / type, path,
                                   std::filesystem::copy_options::overwrite_existing);
        std::filesystem::resize_file(path, 12);
        reject();
        std::filesystem::copy_file(root / "build/assets" / type, path,
                                   std::filesystem::copy_options::overwrite_existing);
        {
            std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
            file.seekp(4);
            file.put(99);
        }
        reject();
        std::filesystem::remove(path);
        reject();
    }
    const auto invalidTexture = temporary / "invalid.rgba";
    writeTexture(invalidTexture, {0, 1, {}});
    bool rejected = false;
    try
    {
        readTexture(invalidTexture);
    }
    catch (...)
    {
        rejected = true;
    }
    assert(rejected);
    std::cout << "Prepared shuttle, " << textures
              << " RGBA textures, shader sources and corrupt asset checks passed\n";
}
