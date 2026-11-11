// ImGui/ImPlot backend setup and frame rendering.
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include <stdexcept>

#include "ui/UiManager.h"

UiManager::UiManager(GLFWwindow* window) {
    if (window == nullptr) {
        throw std::invalid_argument("UiManager requires a GLFW window");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
    ImPlot::StyleColorsDark();

    // The application does not need persisted window layout; not writing an
    // ini file also keeps runs from leaving files in the working directory.
    ImGui::GetIO().IniFilename = nullptr;

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        throw std::runtime_error("Failed to initialize the ImGui GLFW backend");
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        throw std::runtime_error("Failed to initialize the ImGui OpenGL backend");
    }

    float xScale = 1.f;
    float yScale = 1.f;
    glfwGetWindowContentScale(window, &xScale, &yScale);
    if (xScale > 0.f) {
        ImGui::GetIO().FontGlobalScale = xScale;
    }
}

UiManager::~UiManager() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}

void UiManager::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UiManager::render() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool UiManager::wantsMouse() const {
    return ImGui::GetIO().WantCaptureMouse;
}

bool UiManager::wantsKeyboard() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}
