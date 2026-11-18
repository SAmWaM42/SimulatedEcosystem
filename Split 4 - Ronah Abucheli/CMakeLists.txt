cmake_minimum_required(VERSION 4.2.3)
project(SimulatedEcosystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- GLFW (build from source for MinGW compatibility) ---
include(FetchContent)
FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# --- OpenGL ---
find_package(OpenGL REQUIRED)

# System libraries needed by GLFW on Windows
if(WIN32)
    set(SYSTEM_LIBS opengl32)
else()
    set(SYSTEM_LIBS ${OPENGL_LIBRARIES})
endif()

add_executable(program)

target_sources(program
    PRIVATE
        ecosystem.c++
        glad/glad.c
        Geometry/primitives.c++
        Geometry/mesh.cpp
        Rendering/camera.cpp
        Rendering/renderer.cpp
        Managers/scene_manager.cpp
)

target_link_libraries(program
    PRIVATE
        glfw
        ${SYSTEM_LIBS}
)

target_include_directories(program PRIVATE
    glad
    Components
    Entities
    Geometry
    Managers
    Rendering
    Inc
    Tests
)

target_compile_options(program PRIVATE
    -Wall
    -Wextra
)
