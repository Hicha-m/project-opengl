cmake_minimum_required(VERSION 3.20)
project(OpenGLSpaceCinematic VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CTest)
option(ENABLE_RUNTIME_TESTS "Register the OpenGL tests (requires a display)" OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(SDL3 3.2 CONFIG REQUIRED)
find_program(FFMPEG_EXECUTABLE ffmpeg REQUIRED)

add_library(project_core STATIC
    src/Application.cpp
    src/audio/MusicPlayer.cpp
    src/camera/Camera.cpp
    src/camera/CinematicCamera.cpp
    src/graphics/Mesh.cpp
    src/graphics/ShaderProgram.cpp
    src/graphics/Texture2D.cpp
    src/graphics/Renderer.cpp
    src/geometry/Sphere.cpp
    src/scene/LightManager.cpp
    src/scene/SceneSetup.cpp
    src/cinematic/MainSequence.cpp
    src/systems/MeteorResources.cpp
    src/systems/MeteorSystem.cpp
    src/systems/SolarSystem.cpp
    src/systems/MeteorShower.cpp
    src/systems/ImpactLightSystem.cpp
    src/systems/EarthDamageSystem.cpp
    src/systems/EarthBreakupSystem.cpp
    src/graphics/HDRPipeline.cpp
    src/systems/ParticleSystem.cpp
    src/systems/MeteorTrailEmitter.cpp
    src/systems/ParticleEmitter.cpp
    src/systems/ImpactParticleEmitter.cpp
    src/graphics/ParticleRenderer.cpp
    src/animation/Easing.cpp
    src/animation/Timeline.cpp
)
target_include_directories(project_core PUBLIC src)
target_link_libraries(project_core PUBLIC OpenGL::GL glfw GLEW::GLEW glm::glm SDL3::SDL3)
if(MSVC)
    target_compile_options(project_core PRIVATE /W4)
else()
    target_compile_options(project_core PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# Preserve the existing relative paths in a self-contained runtime directory.
set(RUNTIME_DIR "${CMAKE_CURRENT_BINARY_DIR}/runtime")
add_custom_target(runtime_assets ALL)
foreach(directory shaders textures models)
    add_custom_command(TARGET runtime_assets POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/${directory}" "${RUNTIME_DIR}/${directory}"
        VERBATIM)
endforeach()

function(convert_audio input output)
    add_custom_command(OUTPUT "${RUNTIME_DIR}/build/music/${output}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${RUNTIME_DIR}/build/music"
        COMMAND "${FFMPEG_EXECUTABLE}" -v error -y
            -i "${CMAKE_CURRENT_SOURCE_DIR}/audio/${input}"
            -ar 48000 -ac 2 -c:a pcm_s16le "${RUNTIME_DIR}/build/music/${output}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/audio/${input}"
        VERBATIM)
endfunction()
convert_audio("Can You Hear The Music.mp3" cinematic.wav)
convert_audio("asteroid-hitting-something.mp3" impact.wav)
add_custom_target(runtime_audio ALL DEPENDS
    "${RUNTIME_DIR}/build/music/cinematic.wav"
    "${RUNTIME_DIR}/build/music/impact.wav")

add_executable(project src/main.cpp)
target_link_libraries(project PRIVATE project_core)
set_target_properties(project PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${RUNTIME_DIR}")
add_dependencies(project runtime_assets runtime_audio)
add_custom_target(run
    COMMAND "$<TARGET_FILE:project>"
    WORKING_DIRECTORY "${RUNTIME_DIR}"
    DEPENDS project USES_TERMINAL)

if(BUILD_TESTING)
    set(TEST_NAMES music_player solar_system timeline main_sequence meteor_system meteor_shower meteor_collision impact_light particle_system meteor_trail earth_damage destruction_level earth_breakup application)
    foreach(name IN LISTS TEST_NAMES)
        add_executable(test_${name} tests/${name}.cpp)
        target_link_libraries(test_${name} PRIVATE project_core)
        # The existing tests use assert(), including for operations with side effects.
        if(MSVC)
            target_compile_options(test_${name} PRIVATE /UNDEBUG)
        else()
            target_compile_options(test_${name} PRIVATE -UNDEBUG)
        endif()
        add_dependencies(test_${name} runtime_assets runtime_audio)
        if(NOT name STREQUAL "application" OR ENABLE_RUNTIME_TESTS)
            add_test(NAME ${name} COMMAND test_${name})
            set_tests_properties(${name} PROPERTIES
                WORKING_DIRECTORY "${RUNTIME_DIR}"
                ENVIRONMENT "SDL_AUDIO_DRIVER=dummy"
                TIMEOUT 120)
            if(name STREQUAL "application")
                set_tests_properties(${name} PROPERTIES LABELS runtime TIMEOUT 600 RUN_SERIAL TRUE)
            else()
                set_tests_properties(${name} PROPERTIES LABELS unit)
            endif()
        endif()
    endforeach()
endif()

# Install a runnable directory; shared dependencies are provided by the host/container.
install(TARGETS project RUNTIME DESTINATION .)
install(DIRECTORY shaders textures models DESTINATION .)
install(FILES "${RUNTIME_DIR}/build/music/cinematic.wav"
    "${RUNTIME_DIR}/build/music/impact.wav" DESTINATION build/music)
