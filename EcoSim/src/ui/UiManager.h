// Dear ImGui/ImPlot lifecycle wrapper: owns the contexts and GLFW/OpenGL3
// backends, and exposes the per-frame begin/render calls used by the app.
#pragma once

struct GLFWwindow;

// Sets up ImGui and ImPlot for one existing GLFW window.
class UiManager {
public:
    // Creates the contexts and backends; construct after the Window so the
    // backends can chain the application's existing GLFW callbacks.
    explicit UiManager(GLFWwindow* window);
    // Shuts down the backends and destroys the ImPlot and ImGui contexts.
    ~UiManager();

    UiManager(const UiManager&) = delete;
    UiManager& operator=(const UiManager&) = delete;
    UiManager(UiManager&&) = delete;
    UiManager& operator=(UiManager&&) = delete;

    // Starts a new ImGui frame (backends and ImGui::NewFrame).
    void beginFrame();
    // Renders the recorded ImGui draw data into the current OpenGL framebuffer.
    void render();
    // True when ImGui wants mouse input, so the 3D view must ignore it.
    bool wantsMouse() const;
    // True when ImGui wants keyboard input, so shortcuts must be ignored.
    bool wantsKeyboard() const;
};
