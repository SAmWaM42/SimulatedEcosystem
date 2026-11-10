// GLFW window, context, and input wrapper. Its interface keeps GLFW types
// opaque so only the viewer's Window implementation depends on GL headers.
#pragma once

#include <string>

#include "render/Input.h"

struct GLFWwindow;

// Owns one GLFW window and converts its raw input into an InputState snapshot.
class Window {
public:
    // Creates an OpenGL 3.3 core window, loads GLAD, and enables vsync.
    Window(int width, int height, const std::string& title);
    // Destroys the GLFW window and terminates GLFW.
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    // True when the user or ESC requested window close.
    bool shouldClose() const;
    // Polls events and refreshes the per-frame input snapshot.
    void pollEvents();
    // Presents the rendered back buffer.
    void swapBuffers();
    // Enables depth testing and counter-clockwise back-face culling.
    void enableDepthAndCull();
    // Clears colour and depth using the given sky colour.
    void clear(float red, float green, float blue);
    // Starts the translucent pass: blending on, depth writes off.
    void beginBlendPass();
    // Ends the translucent pass: depth writes on, blending off.
    void endBlendPass();
    // Returns the latest input snapshot.
    InputState input() const;
    // Framebuffer size in pixels.
    int width() const;
    int height() const;
    // Framebuffer aspect ratio.
    float aspect() const;
    // Changes the window title.
    void setTitle(const std::string& title);
    // Seconds since GLFW was initialized.
    double time() const;
    // Raw GLFW handle for Dear ImGui integration in later splits.
    GLFWwindow* handle() const;

private:
    static void framebufferSizeCallback(GLFWwindow* window, int width,
                                        int height);
    static void scrollCallback(GLFWwindow* window, double xoffset,
                               double yoffset);

    GLFWwindow* windowHandle = nullptr;
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    InputState inputState;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    double scrollAccumulator = 0.0;
};
