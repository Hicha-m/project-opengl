CXX := g++
CXXFLAGS := -Wall -Wextra -Wno-unused-parameter -std=c++17 -I. $(shell pkg-config --cflags glfw3 glew)
LDLIBS := $(shell pkg-config --libs glfw3 glew) -lGL

TARGET := project
SCENE ?= src/main
COMMON_SRC := Camera.cpp Mesh.cpp ShaderProgram.cpp Texture2D.cpp src/Sphere.cpp
SRC := $(SCENE).cpp $(COMMON_SRC)

.PHONY: all run clean

all: $(TARGET) run

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
