# syntax=docker/dockerfile:1
FROM ubuntu:24.04 AS builder
ARG SDL_VERSION=3.2.10
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build pkg-config curl ca-certificates ffmpeg \
    libgl1-mesa-dev libglfw3-dev libglew-dev libglm-dev libstb-dev libasound2-dev \
    libpulse-dev libx11-dev libxext-dev libxrandr-dev libxrender-dev \
    libxfixes-dev libxi-dev libxss-dev libxtst-dev xvfb xauth \
    && rm -rf /var/lib/apt/lists/*

# Ubuntu 24.04 does not ship SDL3; build a fixed release into /opt/sdl.
RUN curl -fsSL "https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL3-${SDL_VERSION}.tar.gz" \
      -o /tmp/sdl.tar.gz \
    && mkdir /tmp/sdl \
    && tar -xzf /tmp/sdl.tar.gz -C /tmp/sdl --strip-components=1 \
    && cmake -S /tmp/sdl -B /tmp/sdl-build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/sdl \
      -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF \
      -DSDL_X11=ON -DSDL_WAYLAND=OFF \
    && cmake --build /tmp/sdl-build --parallel 2 \
    && cmake --install /tmp/sdl-build
WORKDIR /src
COPY . .
RUN cmake -S . -B build/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=/opt/sdl -DENABLE_RUNTIME_TESTS=ON \
    && cmake --build build/cmake --parallel 2 \
    && ctest --test-dir build/cmake -L unit --output-on-failure \
    && LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a \
      ctest --test-dir build/cmake -L runtime --output-on-failure \
    && cmake --install build/cmake --prefix /opt/project

FROM ubuntu:24.04 AS runtime
RUN apt-get update && apt-get install -y --no-install-recommends \
    libgl1 libgl1-mesa-dri libglx-mesa0 libglfw3 libglew2.2 \
    libasound2t64 libpulse0 && rm -rf /var/lib/apt/lists/*
COPY --from=builder /opt/sdl/lib/libSDL3.so* /usr/local/lib/
RUN ldconfig
WORKDIR /app
COPY --from=builder /opt/project/ ./
CMD ["./project"]
