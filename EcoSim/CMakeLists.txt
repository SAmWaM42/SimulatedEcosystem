# Build configuration for the EcoSim genetics scaffold and its tests.
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
# doctest's CMake file predates CMake 4's minimum-policy requirement; this
# variable is ignored by older CMake versions.
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11)
FetchContent_MakeAvailable(doctest)

add_executable(genetics_tests
    tests/genetics_tests.cpp
)
target_link_libraries(genetics_tests PRIVATE eco_genetics doctest::doctest)

enable_testing()
add_test(NAME genetics COMMAND genetics_tests)
