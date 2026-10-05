CXX := g++
CPPFLAGS := -Isrc -Ithird_party/stb -Ithird_party/dr_libs -Ithird_party/glad/include -DGLFW_INCLUDE_NONE $(shell pkg-config --cflags glfw3 sdl3)
CXXFLAGS := -Wall -Wextra -Wno-unused-parameter -std=c++17
LDLIBS := $(shell pkg-config --libs glfw3 sdl3) -lGL

TARGET := project
SCENE ?= src/main
BUILD_DIR := build
MUSIC_MP3 := $(BUILD_DIR)/music/cinematic.mp3
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
MAIN_OBJ := $(BUILD_DIR)/$(SCENE).o
TEST_NAMES := music_player solar_system timeline main_sequence meteor_system meteor_shower meteor_collision impact_light particle_system meteor_trail earth_damage destruction_level earth_breakup application
TEST_OBJ := $(TEST_NAMES:%=$(BUILD_DIR)/tests/%.o)
TEST_BIN := $(TEST_NAMES:%=$(BUILD_DIR)/tests/%)
DEPS := $(COMMON_OBJ:.o=.d) $(MAIN_OBJ:.o=.d) $(TEST_OBJ:.o=.d)

.PHONY: all run clean test test-sequence test-runtime
all: $(TARGET)

$(MUSIC_MP3): audio/Can\ You\ Hear\ The\ Music.mp3
	@mkdir -p $(dir $@)
	cp "$<" "$@"

$(IMPACT_WAV): audio/asteroid-hitting-something.mp3
	@mkdir -p $(dir $@)
	ffmpeg -v error -y -i "$<" -ar 48000 -ac 2 -c:a pcm_s16le "$@"

$(TARGET): $(MAIN_OBJ) $(COMMON_OBJ) | $(MUSIC_MP3) $(IMPACT_WAV)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -MMD -MP -c $< -o $@

$(TEST_BIN): $(BUILD_DIR)/tests/%: $(BUILD_DIR)/tests/%.o $(COMMON_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

test: $(BUILD_DIR)/tests/music_player $(MUSIC_MP3) $(IMPACT_WAV) $(BUILD_DIR)/tests/solar_system $(BUILD_DIR)/tests/earth_breakup $(BUILD_DIR)/tests/destruction_level $(BUILD_DIR)/tests/earth_damage $(BUILD_DIR)/tests/meteor_trail $(BUILD_DIR)/tests/particle_system $(BUILD_DIR)/tests/timeline $(BUILD_DIR)/tests/main_sequence $(BUILD_DIR)/tests/meteor_system $(BUILD_DIR)/tests/meteor_shower $(BUILD_DIR)/tests/meteor_collision $(BUILD_DIR)/tests/impact_light
	SDL_AUDIO_DRIVER=dummy ./$(BUILD_DIR)/tests/music_player
	./$(BUILD_DIR)/tests/solar_system
	./$(BUILD_DIR)/tests/timeline
	./$(BUILD_DIR)/tests/main_sequence
	./$(BUILD_DIR)/tests/meteor_system
	./$(BUILD_DIR)/tests/meteor_shower
	./$(BUILD_DIR)/tests/meteor_collision
	./$(BUILD_DIR)/tests/impact_light
	./$(BUILD_DIR)/tests/particle_system
	./$(BUILD_DIR)/tests/meteor_trail
	./$(BUILD_DIR)/tests/earth_damage
	./$(BUILD_DIR)/tests/destruction_level
	./$(BUILD_DIR)/tests/earth_breakup

test-sequence: $(BUILD_DIR)/tests/main_sequence
	./$(BUILD_DIR)/tests/main_sequence

$(BUILD_DIR)/tests/music_player: | $(MUSIC_MP3) $(IMPACT_WAV)

test-runtime: $(BUILD_DIR)/tests/application $(MUSIC_MP3) $(IMPACT_WAV)
	./$(BUILD_DIR)/tests/application

clean:
	$(RM) -r $(BUILD_DIR) $(TARGET)

-include $(DEPS)
