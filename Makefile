CXX := g++
CPPFLAGS := -Isrc $(shell pkg-config --cflags glfw3 glew)
CXXFLAGS := -Wall -Wextra -Wno-unused-parameter -std=c++17
LDLIBS := $(shell pkg-config --libs glfw3 glew) -lGL

TARGET := project
SCENE ?= src/main
BUILD_DIR := build

COMMON_SRC := \
	src/Application.cpp \
	src/camera/Camera.cpp \
	src/camera/CinematicCamera.cpp \
	src/graphics/Mesh.cpp \
	src/graphics/ShaderProgram.cpp \
	src/graphics/Texture2D.cpp \
	src/graphics/Renderer.cpp \
	src/geometry/Sphere.cpp \
	src/scene/LightManager.cpp \
	src/scene/SceneSetup.cpp \
	src/cinematic/MainSequence.cpp \
	src/systems/MeteorResources.cpp \
	src/systems/MeteorSystem.cpp \
	src/systems/MeteorShower.cpp \
	src/systems/ImpactLightSystem.cpp \
	src/systems/ParticleSystem.cpp \
	src/systems/ParticleEmitter.cpp \
	src/systems/ImpactParticleEmitter.cpp \
	src/graphics/ParticleRenderer.cpp \
	src/animation/Easing.cpp \
	src/animation/Timeline.cpp

COMMON_OBJ := $(COMMON_SRC:%.cpp=$(BUILD_DIR)/%.o)
MAIN_OBJ := $(BUILD_DIR)/$(SCENE).o
TEST_NAMES := timeline main_sequence meteor_system meteor_shower meteor_collision impact_light particle_system application
TEST_OBJ := $(TEST_NAMES:%=$(BUILD_DIR)/tests/%.o)
TEST_BIN := $(TEST_NAMES:%=$(BUILD_DIR)/tests/%)
DEPS := $(COMMON_OBJ:.o=.d) $(MAIN_OBJ:.o=.d) $(TEST_OBJ:.o=.d)

.PHONY: all run clean test test-sequence test-runtime
all: $(TARGET)

$(TARGET): $(MAIN_OBJ) $(COMMON_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(TEST_BIN): $(BUILD_DIR)/tests/%: $(BUILD_DIR)/tests/%.o $(COMMON_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

test: $(BUILD_DIR)/tests/particle_system $(BUILD_DIR)/tests/timeline $(BUILD_DIR)/tests/main_sequence $(BUILD_DIR)/tests/meteor_system $(BUILD_DIR)/tests/meteor_shower $(BUILD_DIR)/tests/meteor_collision $(BUILD_DIR)/tests/impact_light
	./$(BUILD_DIR)/tests/timeline
	./$(BUILD_DIR)/tests/main_sequence
	./$(BUILD_DIR)/tests/meteor_system
	./$(BUILD_DIR)/tests/meteor_shower
	./$(BUILD_DIR)/tests/meteor_collision
	./$(BUILD_DIR)/tests/impact_light
	./$(BUILD_DIR)/tests/particle_system

test-sequence: $(BUILD_DIR)/tests/main_sequence
	./$(BUILD_DIR)/tests/main_sequence

test-runtime: $(BUILD_DIR)/tests/application
	./$(BUILD_DIR)/tests/application

clean:
	$(RM) -r $(BUILD_DIR) $(TARGET)

-include $(DEPS)
