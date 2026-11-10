// GLFW/GLAD window implementation: context creation, resize/scroll callbacks,
// keyboard/mouse mapping, and per-frame input accumulation.
#define GLFW_INCLUDE_NONE
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "render/Window.h"

#include <stdexcept>
#include <string>

Window::Window(int width, int height, const std::string& title) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    windowHandle = glfwCreateWindow(width, height, title.c_str(), nullptr,
                                    nullptr);
    if (windowHandle == nullptr) {
        glfwTerminate();
        throw std::runtime_error("Failed to create a GLFW window");
    }

    glfwMakeContextCurrent(windowHandle);
    glfwSwapInterval(1);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        glfwDestroyWindow(windowHandle);
        windowHandle = nullptr;
        glfwTerminate();
        throw std::runtime_error("Failed to load OpenGL through GLAD");
    }

    glfwSetWindowUserPointer(windowHandle, this);
    glfwSetFramebufferSizeCallback(windowHandle, framebufferSizeCallback);
    glfwSetScrollCallback(windowHandle, scrollCallback);

    glfwGetFramebufferSize(windowHandle, &framebufferWidth,
                           &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glfwGetCursorPos(windowHandle, &lastMouseX, &lastMouseY);
}

Window::~Window() {
    if (windowHandle != nullptr) {
        glfwDestroyWindow(windowHandle);
    }
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(windowHandle) != 0;
}

void Window::pollEvents() {
    glfwPollEvents();

    if (glfwGetKey(windowHandle, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(windowHandle, GLFW_TRUE);
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(windowHandle, &mouseX, &mouseY);
    inputState.mouseDelta = glm::vec2(
        static_cast<float>(mouseX - lastMouseX),
        static_cast<float>(mouseY - lastMouseY));
    lastMouseX = mouseX;
    lastMouseY = mouseY;

    inputState.moveForward =
        glfwGetKey(windowHandle, GLFW_KEY_W) == GLFW_PRESS ||
        glfwGetKey(windowHandle, GLFW_KEY_UP) == GLFW_PRESS;
    inputState.moveBack =
        glfwGetKey(windowHandle, GLFW_KEY_S) == GLFW_PRESS ||
        glfwGetKey(windowHandle, GLFW_KEY_DOWN) == GLFW_PRESS;
    inputState.moveLeft =
        glfwGetKey(windowHandle, GLFW_KEY_A) == GLFW_PRESS ||
        glfwGetKey(windowHandle, GLFW_KEY_LEFT) == GLFW_PRESS;
    inputState.moveRight =
        glfwGetKey(windowHandle, GLFW_KEY_D) == GLFW_PRESS ||
        glfwGetKey(windowHandle, GLFW_KEY_RIGHT) == GLFW_PRESS;
    inputState.fast =
        glfwGetKey(windowHandle, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    inputState.orbitDrag =
        glfwGetMouseButton(windowHandle, GLFW_MOUSE_BUTTON_LEFT) ==
        GLFW_PRESS;
    inputState.panDrag =
        glfwGetMouseButton(windowHandle, GLFW_MOUSE_BUTTON_RIGHT) ==
            GLFW_PRESS ||
        glfwGetMouseButton(windowHandle, GLFW_MOUSE_BUTTON_MIDDLE) ==
            GLFW_PRESS;

    inputState.scroll = static_cast<float>(scrollAccumulator);
    scrollAccumulator = 0.0;
}

void Window::swapBuffers() {
    glfwSwapBuffers(windowHandle);
}

void Window::enableDepthAndCull() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
}

void Window::clear(float red, float green, float blue) {
    glClearColor(red, green, blue, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::beginBlendPass() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
}

void Window::endBlendPass() {
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

InputState Window::input() const {
    return inputState;
}

int Window::width() const {
    return framebufferWidth;
}

int Window::height() const {
    return framebufferHeight;
}

float Window::aspect() const {
    return (framebufferHeight > 0)
               ? static_cast<float>(framebufferWidth) /
                     static_cast<float>(framebufferHeight)
               : 1.f;
}

void Window::setTitle(const std::string& title) {
    glfwSetWindowTitle(windowHandle, title.c_str());
}

double Window::time() const {
    return glfwGetTime();
}

GLFWwindow* Window::handle() const {
    return windowHandle;
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width,
                                     int height) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self != nullptr) {
        self->framebufferWidth = width;
        self->framebufferHeight = height;
        glViewport(0, 0, width, height);
    }
}

void Window::scrollCallback(GLFWwindow* window, double xoffset,
                            double yoffset) {
    (void)xoffset;
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self != nullptr) {
        self->scrollAccumulator += yoffset;
    }
}
