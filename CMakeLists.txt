cmake_minimum_required(VERSION 4.2.3)
project(SimulatedEcosystem)
# minimimum cmake declaration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(program)
# any c++ file you need run  add here
target_sources(program
  PRIVATE
    ecosystem.c++
    glad/glad.c
)
# any libraries you need basically check compile flags needed
target_link_libraries(program
    PRIVATE
        glfw
        GL)
# folders that have header files that you want to be found
target_include_directories(program PRIVATE
    glad
    Components
    Entities
    Geometry
    Managers
    Rendering
    Inc
)
# compile option flags i think
target_compile_options(program PRIVATE
    -Wall
    -Wextra
)

