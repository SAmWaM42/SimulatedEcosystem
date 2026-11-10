// GL-free orbit camera for the EcoSim viewer; input is supplied as a plain
// snapshot so the camera can be unit-tested without GLFW.
#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "render/Input.h"

class Terrain;

// Orbit camera with terrain-aware target and eye constraints.
class Camera {
public:
    // Starts looking at the terrain centre with the default orbit.
    explicit Camera(const Terrain& terrain);
    // Applies one frame of input and keeps target and eye valid for the terrain.
    void update(const InputState& in, float dt, const Terrain& terrain);
    // View matrix from eye to target.
    glm::mat4 viewMatrix() const;
    // Perspective projection with a 50 degree vertical field of view.
    glm::mat4 projectionMatrix(float aspect) const;
    // Camera position derived from target, yaw, pitch, and distance.
    glm::vec3 eye() const;
    // Current orbit target.
    glm::vec3 target() const;
    float distance() const;
    float pitch() const;
    float yaw() const;
    // Replaces the orbit target, e.g. to follow an organism later.
    void setTarget(const glm::vec3& t);

private:
    glm::vec3 orbitTarget;
    float orbitYaw;
    float orbitPitch;
    float orbitDistance;
};
