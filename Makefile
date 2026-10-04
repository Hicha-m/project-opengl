CXX := g++
CXXFLAGS := -Wall -Wextra -Wno-unused-parameter -std=c++17 -I. $(shell pkg-config --cflags glfw3 glew)
LDLIBS := $(shell pkg-config --libs glfw3 glew) -lGL

TARGET := project
SCENE ?= src/main
COMMON_SRC := Camera.cpp Mesh.cpp ShaderProgram.cpp Texture2D.cpp src/Sphere.cpp src/graphics/Renderer.cpp src/scene/LightManager.cpp src/animation/CinematicCamera.cpp src/animation/Easing.cpp src/animation/Timeline.cpp src/Application.cpp src/scene/SceneSetup.cpp src/cinematic/MainSequence.cpp

SRC := $(SCENE).cpp $(COMMON_SRC)

.PHONY: all run clean

all: $(TARGET) run

HEADERS := $(shell rg --files src -g '*.h' -g '!*copy*') $(wildcard *.h)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: test
test: test-sequence
	$(CXX) $(CXXFLAGS) -o /tmp/space-timeline-tests tests/timeline.cpp Camera.cpp src/animation/CinematicCamera.cpp src/animation/Easing.cpp src/animation/Timeline.cpp $(LDLIBS)
	/tmp/space-timeline-tests

.PHONY: test-sequence test-runtime
test-sequence:
	$(CXX) $(CXXFLAGS) -o /tmp/space-sequence-tests tests/main_sequence.cpp Camera.cpp src/animation/CinematicCamera.cpp src/animation/Easing.cpp src/animation/Timeline.cpp src/cinematic/MainSequence.cpp $(LDLIBS)
	/tmp/space-sequence-tests

/tmp/space-application-tests: tests/application.cpp $(COMMON_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ tests/application.cpp $(COMMON_SRC) $(LDLIBS)

test-runtime: /tmp/space-application-tests
	/tmp/space-application-tests
