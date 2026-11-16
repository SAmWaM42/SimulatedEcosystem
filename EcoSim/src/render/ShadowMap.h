// Depth-only shadow map for the ecosystem: renders organism casters from a
// fixed virtual sun and exposes the depth texture to the terrain shader.
#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

// Builds the orthographic light-space matrix that covers the whole world.
glm::mat4 shadowLightSpaceMatrix(const glm::vec3& lightDirection,
                                 const glm::vec3& worldCentre,
                                 float worldSize);

// Owns the depth framebuffer and restores the caller's viewport/framebuffer.
class ShadowMap {
public:
    // Creates a square depth-only shadow map. Throws if the FBO is incomplete.
    explicit ShadowMap(int resolution = 2048);
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    // Binds the depth framebuffer, clears depth, and sets the shadow viewport.
    void beginRender();
    // Restores the framebuffer and viewport saved by beginRender().
    void endRender();
    // Depth texture for sampling from the terrain shader.
    unsigned int texture() const;

private:
    unsigned int framebuffer = 0;
    unsigned int depthTexture = 0;
    int size = 0;
    int previousFramebuffer = 0;
    int previousViewport[4] = {0, 0, 1, 1};
};
