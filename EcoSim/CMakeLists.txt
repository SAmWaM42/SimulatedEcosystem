# Build configuration for EcoSim: genetics, headless simulation, world tools, and the optional OpenGL viewer.
cmake_minimum_required(VERSION 3.20)
project(EcoSim LANGUAGES CXX)

option(ECOSIM_BUILD_VIEWER "Build the OpenGL viewer" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Apply the project warning policy to our targets only so fetched third-party
# sources such as GLFW are not compiled with EcoSim's stricter warning set.
function(ecosim_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endfunction()

# Genetics core is graphics-independent so tests and tools can run headless.
add_library(eco_genetics STATIC
    src/genetics/Genetics.cpp
)
target_include_directories(eco_genetics PUBLIC src)
ecosim_enable_warnings(eco_genetics)

add_executable(genetics_demo
    apps/genetics_demo.cpp
)
target_link_libraries(genetics_demo PRIVATE eco_genetics)
ecosim_enable_warnings(genetics_demo)

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

# GL-free world and rendering data used by both the viewer and headless tests.
add_library(eco_world STATIC
    src/world/Terrain.cpp
    src/render/MeshFactory.cpp
    src/render/Camera.cpp
    src/render/Assets.cpp
)
target_link_libraries(eco_world PUBLIC glm::glm)
target_include_directories(eco_world PUBLIC src)
ecosim_enable_warnings(eco_world)

# Headless simulation foundation; it depends on genetics and GLM only.
add_library(eco_sim STATIC
    src/sim/SpatialGrid.cpp
    src/sim/Organism.cpp
    src/sim/Simulation.cpp
    src/sim/Reproduction.cpp
    src/sim/Lineage.cpp
    src/sim/Stats.cpp
    src/sim/SimClock.cpp
)
target_link_libraries(eco_sim PUBLIC eco_genetics glm::glm eco_world)
target_include_directories(eco_sim PUBLIC src)
ecosim_enable_warnings(eco_sim)

# GL-free picking and UI text/chart preparation are shared by the viewer, the
# main app, tests, and the ImGui panels.
add_library(eco_view STATIC
    src/render/Picking.cpp
    src/ui/UiText.cpp
    src/ui/ChartData.cpp
    src/ui/LineageLayout.cpp
    src/ui/Punnett.cpp
)
target_link_libraries(eco_view PUBLIC eco_sim eco_world)
target_include_directories(eco_view PUBLIC src)
ecosim_enable_warnings(eco_view)

# GL-free tooling used by screenshots and tests.
add_library(eco_tools STATIC
    src/tools/PngWriter.cpp
)
target_include_directories(eco_tools PUBLIC src)
ecosim_enable_warnings(eco_tools)

add_executable(sim_headless
    apps/sim_headless.cpp
)
target_link_libraries(sim_headless PRIVATE eco_sim)
ecosim_enable_warnings(sim_headless)

add_executable(balance_run
    apps/balance_run.cpp
)
target_link_libraries(balance_run PRIVATE eco_sim)
ecosim_enable_warnings(balance_run)

add_executable(genetics_tests
    tests/genetics_tests.cpp
)
target_link_libraries(genetics_tests PRIVATE eco_genetics doctest::doctest)
ecosim_enable_warnings(genetics_tests)

add_executable(sim_tests
    tests/sim_tests.cpp
)
target_link_libraries(sim_tests PRIVATE eco_sim doctest::doctest)
ecosim_enable_warnings(sim_tests)

add_executable(behaviour_tests
    tests/behaviour_tests.cpp
)
target_link_libraries(behaviour_tests PRIVATE eco_sim doctest::doctest)
ecosim_enable_warnings(behaviour_tests)

add_executable(reproduction_tests
    tests/reproduction_tests.cpp
)
target_link_libraries(reproduction_tests PRIVATE eco_sim doctest::doctest)
ecosim_enable_warnings(reproduction_tests)

add_executable(world_tests
    tests/world_tests.cpp
)
target_link_libraries(world_tests PRIVATE eco_world doctest::doctest)
ecosim_enable_warnings(world_tests)

add_executable(view_tests
    tests/view_tests.cpp
)
target_link_libraries(view_tests PRIVATE eco_view eco_sim eco_world doctest::doctest)
ecosim_enable_warnings(view_tests)

add_executable(ui_tests
    tests/ui_tests.cpp
)
target_link_libraries(ui_tests PRIVATE eco_view doctest::doctest)
ecosim_enable_warnings(ui_tests)

add_executable(png_tests
    tests/png_tests.cpp
)
target_link_libraries(png_tests PRIVATE eco_tools doctest::doctest)
ecosim_enable_warnings(png_tests)

if(ECOSIM_BUILD_VIEWER)
    enable_language(C)

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4)
    FetchContent_MakeAvailable(glfw)

    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.90.9)
    FetchContent_Declare(implot
        GIT_REPOSITORY https://github.com/epezent/implot.git
        GIT_TAG v0.16)
    FetchContent_MakeAvailable(imgui implot)

    # Neither dependency ships a usable CMake project, so define the libraries
    # here without applying EcoSim's warning policy to third-party code.
    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui PUBLIC glfw)
    if(CMAKE_DL_LIBS)
        target_link_libraries(imgui PUBLIC ${CMAKE_DL_LIBS})
    endif()

    add_library(implot STATIC
        ${implot_SOURCE_DIR}/implot.cpp
        ${implot_SOURCE_DIR}/implot_items.cpp
    )
    target_include_directories(implot PUBLIC ${implot_SOURCE_DIR})
    target_link_libraries(implot PUBLIC imgui)

    add_library(glad STATIC
        third_party/glad/src/gl.c
    )
    target_include_directories(glad PUBLIC third_party/glad/include)
    if(CMAKE_DL_LIBS)
        target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})
    endif()

    add_library(eco_render STATIC
        src/render/Window.cpp
        src/render/Shader.cpp
        src/render/Mesh.cpp
        src/render/DebugDraw.cpp
        src/render/SimRenderer.cpp
    )
    target_link_libraries(eco_render PUBLIC eco_world eco_view eco_sim glfw glad)
ecosim_enable_warnings(eco_render)

    add_library(eco_ui STATIC
        src/ui/UiManager.cpp
        src/ui/Panels.cpp
    )
    target_link_libraries(eco_ui PUBLIC eco_view eco_sim imgui implot)
    ecosim_enable_warnings(eco_ui)

    add_executable(ecosim_viewer
        apps/ecosim_viewer.cpp
    )
    target_link_libraries(ecosim_viewer PRIVATE eco_render)
ecosim_enable_warnings(ecosim_viewer)

    add_custom_command(TARGET ecosim_viewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:ecosim_viewer>/assets"
        COMMENT "Copying EcoSim assets next to the viewer")

    add_executable(ecosim
        apps/ecosim.cpp
    )
    target_link_libraries(ecosim PRIVATE eco_render eco_ui eco_tools)
    ecosim_enable_warnings(ecosim)

    add_custom_command(TARGET ecosim POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:ecosim>/assets"
        COMMENT "Copying EcoSim assets next to ecosim")
endif()

enable_testing()
add_test(NAME genetics COMMAND genetics_tests)
add_test(NAME sim COMMAND sim_tests)
add_test(NAME behaviour COMMAND behaviour_tests)
add_test(NAME reproduction COMMAND reproduction_tests)
add_test(NAME world COMMAND world_tests)
add_test(NAME view COMMAND view_tests)
add_test(NAME ui COMMAND ui_tests)
add_test(NAME png COMMAND png_tests)
add_test(NAME balance_smoke COMMAND balance_run 2 300 1)
set_tests_properties(balance_smoke PROPERTIES TIMEOUT 300)




