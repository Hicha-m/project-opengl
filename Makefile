CXX := g++
CXXFLAGS := -Wall -Wextra -Wno-unused-parameter -std=c++17 -I. $(shell pkg-config --cflags glfw3 glew)
LDLIBS := $(shell pkg-config --libs glfw3 glew) -lGL

TARGET := project
SCENE ?= src/main
COMMON_SRC := Camera.cpp Mesh.cpp ShaderProgram.cpp Texture2D.cpp src/Sphere.cpp src/graphics/Renderer.cpp src/scene/LightManager.cpp src/animation/CinematicCamera.cpp src/animation/Easing.cpp src/animation/Timeline.cpp

SRC := $(SCENE).cpp $(COMMON_SRC)

.PHONY: all run clean

all: $(TARGET) run

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
