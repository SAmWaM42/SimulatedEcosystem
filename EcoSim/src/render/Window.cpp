// GLFW/GLAD window implementation: context creation, resize/scroll callbacks,
// keyboard/mouse mapping, and per-frame input accumulation.
#define GLFW_INCLUDE_NONE
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "render/Window.h"

#include <cmath>
#include <cstddef>
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
    previousKeys.assign(static_cast<std::size_t>(GLFW_KEY_LAST + 1), 0);
    currentKeys.assign(static_cast<std::size_t>(GLFW_KEY_LAST + 1), 0);
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

    previousKeys = currentKeys;
    for (int key = 0; key <= GLFW_KEY_LAST; ++key) {
        currentKeys[static_cast<std::size_t>(key)] =
            (glfwGetKey(windowHandle, key) == GLFW_PRESS) ? 1 : 0;
    }

    inputState.clicked = false;

    if (currentKeys[GLFW_KEY_ESCAPE] != 0) {
        glfwSetWindowShouldClose(windowHandle, GLFW_TRUE);
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(windowHandle, &mouseX, &mouseY);
    inputState.mousePos = glm::vec2(static_cast<float>(mouseX),
                                    static_cast<float>(mouseY));
    inputState.mouseDelta = glm::vec2(
        static_cast<float>(mouseX - lastMouseX),
        static_cast<float>(mouseY - lastMouseY));
    lastMouseX = mouseX;
    lastMouseY = mouseY;

    inputState.moveForward =
        currentKeys[GLFW_KEY_W] != 0 || currentKeys[GLFW_KEY_UP] != 0;
    inputState.moveBack =
        currentKeys[GLFW_KEY_S] != 0 || currentKeys[GLFW_KEY_DOWN] != 0;
    inputState.moveLeft =
        currentKeys[GLFW_KEY_A] != 0 || currentKeys[GLFW_KEY_LEFT] != 0;
    inputState.moveRight =
        currentKeys[GLFW_KEY_D] != 0 || currentKeys[GLFW_KEY_RIGHT] != 0;
    inputState.fast = currentKeys[GLFW_KEY_LEFT_SHIFT] != 0;

    const bool leftDown =
        glfwGetMouseButton(windowHandle, GLFW_MOUSE_BUTTON_LEFT) ==
        GLFW_PRESS;
    inputState.orbitDrag = leftDown;
    inputState.panDrag =
        glfwGetMouseButton(windowHandle, GLFW_MOUSE_BUTTON_RIGHT) ==
            GLFW_PRESS ||
        glfwGetMouseButton(windowHandle, GLFW_MOUSE_BUTTON_MIDDLE) ==
            GLFW_PRESS;

    if (leftDown && !leftMouseDown) {
        clickCandidate = true;
        clickStartX = mouseX;
        clickStartY = mouseY;
    }
    if (!leftDown && leftMouseDown) {
        if (clickCandidate) {
            const double dx = mouseX - clickStartX;
            const double dy = mouseY - clickStartY;
            if (std::sqrt(dx * dx + dy * dy) < 4.0) {
                inputState.clicked = true;
                inputState.clickPos =
                    glm::vec2(static_cast<float>(mouseX),
                              static_cast<float>(mouseY));
            }
        }
        clickCandidate = false;
    }
    leftMouseDown = leftDown;

    inputState.scroll = static_cast<float>(scrollAccumulator);
    scrollAccumulator = 0.0;
}

bool Window::keyPressed(int glfwKey) const {
    if (glfwKey < 0 ||
        static_cast<std::size_t>(glfwKey) >= currentKeys.size()) {
        return false;
    }
    const std::size_t index = static_cast<std::size_t>(glfwKey);
    return currentKeys[index] != 0 && previousKeys[index] == 0;
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
