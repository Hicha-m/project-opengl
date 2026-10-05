CXX := g++
CPPFLAGS := -Isrc -Ithird_party/stb -Ithird_party/glad/include -DGLFW_INCLUDE_NONE $(shell pkg-config --cflags glfw3 sdl3)
CXXFLAGS := -Wall -Wextra -Wno-unused-parameter -std=c++17
LDLIBS := $(shell pkg-config --libs glfw3 sdl3) -lGL

TARGET := project
BUILD_DIR := build
MUSIC_WAV := $(BUILD_DIR)/music/cinematic.wav
IMPACT_WAV := $(BUILD_DIR)/music/impact.wav

COMMON_SRC := \
	src/platform/ResourcePaths.cpp \
	src/Application.cpp \
	src/audio/MusicPlayer.cpp \
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
	src/systems/SolarSystem.cpp \
	src/systems/MeteorShower.cpp \
	src/systems/ImpactLightSystem.cpp \
	src/systems/EarthDamageSystem.cpp \
	src/systems/EarthBreakupSystem.cpp \
	src/graphics/HDRPipeline.cpp \
	src/systems/ParticleSystem.cpp \
	src/systems/MeteorTrailEmitter.cpp \
	src/systems/ParticleEmitter.cpp \
	src/systems/ImpactParticleEmitter.cpp \
	src/graphics/ParticleRenderer.cpp \
	src/animation/Easing.cpp \
	src/animation/Timeline.cpp

COMMON_OBJ := $(COMMON_SRC:%.cpp=$(BUILD_DIR)/%.o) $(BUILD_DIR)/third_party/glad/src/gl.o
MAIN_OBJ := $(BUILD_DIR)/src/main.o
TEST_NAMES := resource_paths music_player solar_system timeline main_sequence meteor_system meteor_shower meteor_collision impact_light particle_system meteor_trail earth_damage destruction_level earth_breakup application
UNIT_TEST_NAMES := $(filter-out application,$(TEST_NAMES))
UNIT_TEST_BIN := $(UNIT_TEST_NAMES:%=$(BUILD_DIR)/tests/%)
TEST_OBJ := $(TEST_NAMES:%=$(BUILD_DIR)/tests/%.o)
TEST_BIN := $(TEST_NAMES:%=$(BUILD_DIR)/tests/%)
DEPS := $(COMMON_OBJ:.o=.d) $(MAIN_OBJ:.o=.d) $(TEST_OBJ:.o=.d)

.PHONY: all run clean test test-sequence test-runtime
all: $(TARGET)

$(MUSIC_WAV): audio/Can\ You\ Hear\ The\ Music.mp3
	@mkdir -p $(dir $@)
	ffmpeg -v error -y -i "$<" -ar 48000 -ac 2 -c:a pcm_s16le "$@"

$(IMPACT_WAV): audio/asteroid-hitting-something.mp3
	@mkdir -p $(dir $@)
	ffmpeg -v error -y -i "$<" -ar 48000 -ac 2 -c:a pcm_s16le "$@"

$(TARGET): $(MAIN_OBJ) $(COMMON_OBJ) | $(MUSIC_WAV) $(IMPACT_WAV)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Tests use assert(), including operations with side effects.
$(TEST_OBJ): CXXFLAGS += -UNDEBUG

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -MMD -MP -c $< -o $@

$(TEST_BIN): $(BUILD_DIR)/tests/%: $(BUILD_DIR)/tests/%.o $(COMMON_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

test: $(UNIT_TEST_BIN) $(MUSIC_WAV) $(IMPACT_WAV)
	@set -e; for test in $(UNIT_TEST_BIN); do SDL_AUDIO_DRIVER=dummy ./$$test; done

test-sequence: $(BUILD_DIR)/tests/main_sequence
	./$(BUILD_DIR)/tests/main_sequence

$(BUILD_DIR)/tests/music_player: | $(MUSIC_WAV) $(IMPACT_WAV)

test-runtime: $(BUILD_DIR)/tests/application $(MUSIC_WAV) $(IMPACT_WAV)
	./$(BUILD_DIR)/tests/application

clean:
	$(RM) -r $(BUILD_DIR) $(TARGET)

-include $(DEPS)
