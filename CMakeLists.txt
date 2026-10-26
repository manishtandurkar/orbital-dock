cmake_minimum_required(VERSION 3.20)

project(OrbitalDock LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)
set(FETCHCONTENT_QUIET ON)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
if(UNIX AND NOT APPLE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
endif()
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_DRACO OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.4.3
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glfw glm assimp imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw)

add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

add_library(stb_image STATIC third_party/stb/stb_image.c)
target_include_directories(stb_image PUBLIC third_party/stb)

add_library(miniaudio STATIC third_party/miniaudio/miniaudio.c)
target_include_directories(miniaudio PUBLIC third_party/miniaudio)
find_package(Threads REQUIRED)
target_link_libraries(miniaudio PUBLIC Threads::Threads)
if(APPLE)
    target_link_libraries(miniaudio PUBLIC
        "-framework CoreFoundation"
        "-framework CoreAudio"
        "-framework AudioToolbox"
    )
elseif(UNIX)
    target_link_libraries(miniaudio PUBLIC ${CMAKE_DL_LIBS} m)
endif()

set(ORBITAL_DOCK_SOURCES
    src/main.cpp
    src/platform/ExecutablePath.cpp
    src/core/Assets.cpp
    src/core/Window.cpp
    src/core/Input.cpp
    src/core/Timer.cpp
    src/core/GameStateMachine.cpp
    src/core/Game.cpp
    src/render/GlDebug.cpp
    src/render/Shader.cpp
    src/render/Mesh.cpp
    src/render/Camera.cpp
    src/render/Texture.cpp
    src/render/Cubemap.cpp
    src/render/Model.cpp
    src/render/ModelLibrary.cpp
    src/render/DebugLines.cpp
    src/render/Renderer.cpp
    src/scene/SceneNode.cpp
    src/physics/Collision.cpp
    src/physics/DockingCheck.cpp
    src/gameplay/Station.cpp
    src/gameplay/Pod.cpp
    src/gameplay/Debris.cpp
    src/gameplay/DebrisField.cpp
    src/gameplay/CollisionResolver.cpp
    src/gameplay/ChaseCamera.cpp
    src/gameplay/EvaController.cpp
    src/gameplay/Mission.cpp
    src/audio/AudioEngine.cpp
    src/ui/ImGuiLayer.cpp
    src/ui/Hud.cpp
    src/ui/Menu.cpp
)

add_executable(orbital_dock ${ORBITAL_DOCK_SOURCES})
target_include_directories(orbital_dock PRIVATE src)
target_link_libraries(orbital_dock PRIVATE glfw glad glm::glm assimp imgui stb_image miniaudio)
target_compile_definitions(orbital_dock PRIVATE GLFW_INCLUDE_NONE GLM_ENABLE_EXPERIMENTAL)

if(MSVC)
    target_compile_options(orbital_dock PRIVATE /W3 /permissive-)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT orbital_dock)
else()
    target_compile_options(orbital_dock PRIVATE -Wall -Wextra)
endif()

add_custom_command(TARGET orbital_dock POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:orbital_dock>/assets
)
