# Bundled dependencies

`stb/stb_image.h` is stb_image 2.30 from <https://github.com/nothings/stb>,
vendored unchanged from the development system's stb package. SHA-256:
`7603b5de9773066b765d2c03e01fe13a6cbaed4d159520f9ba4dde31c9a5733f`.

The complete MIT / public-domain license is included at the end of the header.
The header is bundled so building on Windows, macOS and Linux does not depend
on a distribution-specific `stb/` include directory.

`glad/` contains a reproducibly generated OpenGL 3.3 core loader from glad 2.0.8:

```sh
python -m glad --api gl:core=3.3 --extensions '' --out-path third_party/glad --reproducible c
```

It obtains function pointers from the current GLFW context, including native
Win32/Cocoa, Wayland/EGL and OSMesa contexts. No system GLEW package is required.
The generated-file and Khronos notices are in the headers and `licenses/GLAD.txt`.
