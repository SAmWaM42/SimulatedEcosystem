cmake_minimum_required(VERSION 4.2.3)
project(SimulatedEcosystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
        GL)

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

