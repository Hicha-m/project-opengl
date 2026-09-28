CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17 $(shell pkg-config --cflags glfw3 glew)
LDLIBS := $(shell pkg-config --libs glfw3 glew) -lGL

TARGET := lighting
SCENE ?= Lighting_Phong
COMMON_SRC := Camera.cpp Mesh.cpp ShaderProgram.cpp Texture2D.cpp
SRC := $(SCENE).cpp $(COMMON_SRC)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
