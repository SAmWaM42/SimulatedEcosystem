// Orbit camera implementation: mouse orbit/pan, wheel zoom, WASD movement, and
// terrain/eye clamping.
#include "render/Camera.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "world/Terrain.h"

namespace {
constexpr float kMinPitch = 0.09f;
constexpr float kMaxPitch = 1.52f;
constexpr float kMinDistance = 30.f;
constexpr float kMaxDistance = 4500.f;
constexpr float kOrbitSensitivity = 0.005f;
constexpr float kPanSensitivity = 0.0015f;
constexpr float kMoveSensitivity = 0.8f;
constexpr float kPitchStep = 0.01f;
constexpr float kEyeClearance = 8.f;
}  // namespace

Camera::Camera(const Terrain& terrain)
    : orbitTarget(terrain.config().size * 0.5f, 0.f,
                  terrain.config().size * 0.5f),
      orbitYaw(0.8f),
      orbitPitch(0.7f),
      orbitDistance(1800.f) {
    orbitTarget.y = terrain.heightAt(orbitTarget.x, orbitTarget.z);
}

void Camera::update(const InputState& in, float dt, const Terrain& terrain) {
    if (in.orbitDrag) {
        orbitYaw -= in.mouseDelta.x * kOrbitSensitivity;
        orbitPitch += in.mouseDelta.y * kOrbitSensitivity;
    }

    // Panning moves the world with the cursor, so the target moves opposite
    // the drag direction.
    if (in.panDrag) {
        const float panScale = orbitDistance * kPanSensitivity;
        orbitTarget.x -= in.mouseDelta.x * panScale;
        orbitTarget.z -= in.mouseDelta.y * panScale;
    }

    orbitDistance *= std::pow(0.9f, in.scroll);

    const float forwardInput =
        (in.moveForward ? 1.f : 0.f) - (in.moveBack ? 1.f : 0.f);
    const float strafeInput =
        (in.moveRight ? 1.f : 0.f) - (in.moveLeft ? 1.f : 0.f);
    const glm::vec3 forward = glm::normalize(glm::vec3(
        -std::cos(orbitYaw), 0.f, -std::sin(orbitYaw)));
    const glm::vec3 right(-forward.z, 0.f, forward.x);
    const float fastMultiplier = in.fast ? 3.f : 1.f;
    orbitTarget += (forward * forwardInput + right * strafeInput) *
                   (orbitDistance * kMoveSensitivity * dt * fastMultiplier);

    orbitPitch = std::clamp(orbitPitch, kMinPitch, kMaxPitch);
    orbitDistance = std::clamp(orbitDistance, kMinDistance, kMaxDistance);

    orbitTarget.x = std::clamp(orbitTarget.x, 0.f, terrain.config().size);
    orbitTarget.z = std::clamp(orbitTarget.z, 0.f, terrain.config().size);
    orbitTarget.y = terrain.heightAt(orbitTarget.x, orbitTarget.z);

    // If the eye would clip the terrain, raise the pitch (and therefore the eye)
    // until the clearance is satisfied or the pitch limit is reached.
    for (int attempt = 0; attempt < 200; ++attempt) {
        const glm::vec3 eyePosition = eye();
        if (eyePosition.y >=
            terrain.heightAt(eyePosition.x, eyePosition.z) + kEyeClearance) {
            break;
        }
        if (orbitPitch >= kMaxPitch) {
            break;
        }
        orbitPitch = std::min(kMaxPitch, orbitPitch + kPitchStep);
    }
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(eye(), orbitTarget, glm::vec3(0.f, 1.f, 0.f));
}

glm::mat4 Camera::projectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(50.f), aspect, 1.f, 10000.f);
}

glm::vec3 Camera::eye() const {
    const float cosPitch = std::cos(orbitPitch);
    return orbitTarget +
           orbitDistance * glm::vec3(cosPitch * std::cos(orbitYaw),
                                     std::sin(orbitPitch),
                                     cosPitch * std::sin(orbitYaw));
}

glm::vec3 Camera::target() const {
    return orbitTarget;
}

float Camera::distance() const {
    return orbitDistance;
}

float Camera::pitch() const {
    return orbitPitch;
}

float Camera::yaw() const {
    return orbitYaw;
}

void Camera::setTarget(const glm::vec3& t) {
    orbitTarget = t;
}

Ray Camera::screenRay(glm::vec2 pixel, int width, int height) const {
    const float w = static_cast<float>(width);
    const float h = static_cast<float>(height);
    if (w <= 0.f || h <= 0.f) {
        return {eye(), glm::vec3(0.f, 0.f, -1.f)};
    }

    const glm::mat4 inverseViewProjection =
        glm::inverse(projectionMatrix(w / h) * viewMatrix());
    const float ndcX = 2.f * pixel.x / w - 1.f;
    const float ndcY = 1.f - 2.f * pixel.y / h;

    glm::vec4 nearWorld = inverseViewProjection * glm::vec4(ndcX, ndcY, -1.f, 1.f);
    glm::vec4 farWorld = inverseViewProjection * glm::vec4(ndcX, ndcY, 1.f, 1.f);
    nearWorld /= nearWorld.w;
    farWorld /= farWorld.w;

    Ray ray;
    ray.origin = eye();
    ray.direction = glm::normalize(glm::vec3(farWorld - nearWorld));
    return ray;
}

void Camera::followTarget(glm::vec2 xz, float dt) {
    const float fraction = std::min(1.f, 8.f * dt);
    orbitTarget.x += (xz.x - orbitTarget.x) * fraction;
    orbitTarget.z += (xz.y - orbitTarget.z) * fraction;
}

