# Build configuration for the EcoSim genetics core and headless simulation.
cmake_minimum_required(VERSION 3.20)
project(EcoSim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# Genetics core is graphics-independent so tests and tools can run headless.
add_library(eco_genetics STATIC
    src/genetics/Genetics.cpp
)
target_include_directories(eco_genetics PUBLIC src)

add_executable(genetics_demo
    apps/genetics_demo.cpp
)
target_link_libraries(genetics_demo PRIVATE eco_genetics)

include(FetchContent)
# Some third-party CMake files predate CMake 4's minimum-policy requirement;
# this variable is ignored by older CMake versions.
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11)
FetchContent_MakeAvailable(doctest)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1)
FetchContent_MakeAvailable(glm)

# Headless simulation foundation; it depends on genetics and GLM only.
add_library(eco_sim STATIC
    src/sim/SpatialGrid.cpp
    src/sim/Organism.cpp
    src/sim/Simulation.cpp
    src/sim/Reproduction.cpp
    src/sim/Lineage.cpp
    src/sim/Stats.cpp
)
target_link_libraries(eco_sim PUBLIC eco_genetics glm::glm)
target_include_directories(eco_sim PUBLIC src)

add_executable(sim_headless
    apps/sim_headless.cpp
)
target_link_libraries(sim_headless PRIVATE eco_sim)

add_executable(balance_run
    apps/balance_run.cpp
)
target_link_libraries(balance_run PRIVATE eco_sim)

add_executable(genetics_tests
    tests/genetics_tests.cpp
)
target_link_libraries(genetics_tests PRIVATE eco_genetics doctest::doctest)

add_executable(sim_tests
    tests/sim_tests.cpp
)
target_link_libraries(sim_tests PRIVATE eco_sim doctest::doctest)

add_executable(behaviour_tests
    tests/behaviour_tests.cpp
)
target_link_libraries(behaviour_tests PRIVATE eco_sim doctest::doctest)

add_executable(reproduction_tests
    tests/reproduction_tests.cpp
)
target_link_libraries(reproduction_tests PRIVATE eco_sim doctest::doctest)

enable_testing()
add_test(NAME genetics COMMAND genetics_tests)
add_test(NAME sim COMMAND sim_tests)
add_test(NAME behaviour COMMAND behaviour_tests)
add_test(NAME reproduction COMMAND reproduction_tests)
