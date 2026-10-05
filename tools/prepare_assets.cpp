#include "assets/PreparedAssets.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include "stb_image.h"

int main(int argc, char** argv)
{
    try
    {
        if (argc < 4)
        {
            throw std::runtime_error("Usage: prepare_assets shuttle|texture|shaders OUTPUT INPUT...");
        }
        const std::string mode = argv[1];
        const auto output = std::filesystem::u8path(argv[2]);
        if (mode == "shuttle" && argc == 5)
        {
            writeShuttle(output,
                         prepareShuttle(std::filesystem::u8path(argv[3]), std::filesystem::u8path(argv[4])));
        }
        else if (mode == "texture" && argc == 4)
        {
            int w = 0, h = 0, channels = 0;
            std::unique_ptr<unsigned char, decltype(&stbi_image_free)> pixels(
                stbi_load(argv[3], &w, &h, &channels, 4), stbi_image_free);
            if (!pixels)
            {
                throw std::runtime_error("Cannot decode texture: " + std::string(argv[3]));
            }
            TextureData data;
            data.width = w;
            data.height = h;
            data.pixels.resize(std::size_t(w) * h * 4);
            const std::size_t row = std::size_t(w) * 4;
            for (int y = 0; y < h; ++y)
            {
                std::copy_n(pixels.get() + std::size_t(h - y - 1) * row, row,
                            data.pixels.data() + std::size_t(y) * row);
            }
            writeTexture(output, data);
        }
        else if (mode == "shaders")
        {
            ShaderSources sources;
            for (int i = 3; i < argc; ++i)
            {
                const auto path = std::filesystem::u8path(argv[i]);
                std::ifstream input(path, std::ios::binary);
                if (!input)
                {
                    throw std::runtime_error("Cannot read shader: " + path.string());
                }
                sources["shaders/" + path.filename().u8string()] =
                    std::string(std::istreambuf_iterator<char>(input), {});
            }
            writeShaders(output, sources);
        }
        else
        {
            throw std::runtime_error("Invalid converter arguments");
        }
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
