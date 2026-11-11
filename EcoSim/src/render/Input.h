// Per-frame input snapshot shared by the GLFW window and the GL-free camera.
#pragma once

#include <glm/vec2.hpp>

// GL-independent input state consumed by Camera::update.
struct InputState {
    bool moveForward = false;
    bool moveBack = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool fast = false;
    bool orbitDrag = false;
    bool panDrag = false;
    glm::vec2 mouseDelta{0.f};
    float scroll = 0.f;
    // True for one frame after a left click that did not become a drag.
    bool clicked = false;
    // Pixel positions with origin at the top-left.
    glm::vec2 clickPos{0.f};
    glm::vec2 mousePos{0.f};
};

