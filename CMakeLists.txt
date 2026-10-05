cmake_minimum_required(VERSION 3.21)
if(POLICY CMP0177)
    cmake_policy(SET CMP0177 NEW)
endif()
project(OpenGLSpaceCinematic VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CTest)
option(ENABLE_RUNTIME_TESTS "Register the OpenGL tests (requires a display)" OFF)
option(BUNDLE_RUNTIME_DEPENDENCIES "Include desktop libraries in distributable packages" ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(SDL3 3.2 CONFIG REQUIRED)
find_program(FFMPEG_EXECUTABLE ffmpeg REQUIRED)

add_library(glad STATIC third_party/glad/src/gl.c)
target_include_directories(glad PUBLIC third_party/glad/include)

add_library(project_core STATIC
    src/platform/ResourcePaths.cpp
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
target_include_directories(project_core SYSTEM PRIVATE third_party/stb)
target_include_directories(project_core SYSTEM PRIVATE third_party/dr_libs)
target_link_libraries(project_core PUBLIC OpenGL::GL glfw glad glm::glm SDL3::SDL3)
target_compile_definitions(project_core PUBLIC GLFW_INCLUDE_NONE)
if(MSVC)
    target_compile_options(project_core PRIVATE /W4 /utf-8)
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
add_custom_command(OUTPUT "${RUNTIME_DIR}/build/music/cinematic.mp3"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${RUNTIME_DIR}/build/music"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/audio/Can You Hear The Music.mp3"
        "${RUNTIME_DIR}/build/music/cinematic.mp3"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/audio/Can You Hear The Music.mp3"
    VERBATIM)
convert_audio("asteroid-hitting-something.mp3" impact.wav)
add_custom_target(runtime_audio ALL DEPENDS
    "${RUNTIME_DIR}/build/music/cinematic.mp3"
    "${RUNTIME_DIR}/build/music/impact.wav")
# Remove the previous build's generated music before copying a macOS bundle.
add_custom_command(TARGET runtime_audio POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E rm -f "${RUNTIME_DIR}/build/music/cinematic.wav")

add_executable(project src/main.cpp)
target_link_libraries(project PRIVATE project_core)
# Keep all configurations beside the same assets, including Visual Studio builds.
set_target_properties(project PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${RUNTIME_DIR}/$<0:>")
if(APPLE)
    set_target_properties(project PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.hicha-m.project-opengl"
        MACOSX_BUNDLE_BUNDLE_NAME "OpenGL Space Cinematic"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
    add_custom_command(TARGET project POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${RUNTIME_DIR}/shaders" "$<TARGET_BUNDLE_CONTENT_DIR:project>/Resources/shaders"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${RUNTIME_DIR}/textures" "$<TARGET_BUNDLE_CONTENT_DIR:project>/Resources/textures"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${RUNTIME_DIR}/models" "$<TARGET_BUNDLE_CONTENT_DIR:project>/Resources/models"
        COMMAND "${CMAKE_COMMAND}" -E rm -f "$<TARGET_BUNDLE_CONTENT_DIR:project>/Resources/build/music/cinematic.wav"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${RUNTIME_DIR}/build/music" "$<TARGET_BUNDLE_CONTENT_DIR:project>/Resources/build/music"
        VERBATIM)
elseif(UNIX)
    set_target_properties(project PROPERTIES INSTALL_RPATH "$ORIGIN/lib")
endif()
add_dependencies(project runtime_assets runtime_audio)
add_custom_target(run
    COMMAND "$<TARGET_FILE:project>"
    WORKING_DIRECTORY "${RUNTIME_DIR}"
    DEPENDS project USES_TERMINAL)

if(BUILD_TESTING)
    set(TEST_NAMES resource_paths music_player solar_system timeline main_sequence meteor_system meteor_shower meteor_collision impact_light particle_system meteor_trail earth_damage destruction_level earth_breakup application)
    foreach(name IN LISTS TEST_NAMES)
        add_executable(test_${name} tests/${name}.cpp)
        target_link_libraries(test_${name} PRIVATE project_core)
        # The existing tests use assert(), including for operations with side effects.
        if(MSVC)
            target_compile_options(test_${name} PRIVATE /UNDEBUG /utf-8)
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

if(APPLE)
    install(TARGETS project BUNDLE DESTINATION .)
    set(RESOURCE_DESTINATION "project.app/Contents/Resources")
else()
    install(TARGETS project RUNTIME_DEPENDENCY_SET application_dependencies RUNTIME DESTINATION .)
    set(RESOURCE_DESTINATION ".")
endif()
install(DIRECTORY shaders textures models DESTINATION "${RESOURCE_DESTINATION}")
install(FILES "${RUNTIME_DIR}/build/music/cinematic.mp3" "${RUNTIME_DIR}/build/music/impact.wav"
    DESTINATION "${RESOURCE_DESTINATION}/build/music")
install(FILES README.md DESTINATION .)
install(DIRECTORY third_party/ DESTINATION licenses
    FILES_MATCHING PATTERN "*.txt" PATTERN "*.md" PATTERN "stb_image.h")

if(BUNDLE_RUNTIME_DEPENDENCIES)
    if(APPLE)
        # Rewrites dylib paths to the relocated .app instead of Homebrew/build paths.
        install(CODE "include(BundleUtilities)\nfixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/project.app\" \"\" \"$<TARGET_FILE_DIR:SDL3::SDL3>;$<TARGET_FILE_DIR:glfw>\")")
        # Ad-hoc signing allows the modified bundle to launch on Apple Silicon.
        install(CODE "execute_process(COMMAND codesign --force --deep --sign - \"\${CMAKE_INSTALL_PREFIX}/project.app\" COMMAND_ERROR_IS_FATAL ANY)")
    elseif(WIN32)
        if(NOT VCPKG_TARGET_TRIPLET MATCHES "windows-static$")
            install(RUNTIME_DEPENDENCY_SET application_dependencies
                PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
                POST_EXCLUDE_REGEXES ".*[Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\\\].*"
                RUNTIME DESTINATION .)
        endif()
        # The windows-static triplet and /MT embed the application libraries and CRT.
        # Only Windows' own DLLs remain; do not recursively try to redistribute them.
    else()
        # Graphics drivers and OS libraries come from the host; bundle our desktop libraries.
        install(RUNTIME_DEPENDENCY_SET application_dependencies
            PRE_INCLUDE_REGEXES "^lib(SDL3|glfw)\\.so.*"
            PRE_EXCLUDE_REGEXES ".*"
            LIBRARY DESTINATION lib)
    endif()
endif()

set(CPACK_PACKAGE_NAME "OpenGLSpaceCinematic")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
if(WIN32 OR APPLE)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()
include(CPack)
