// Shadow-map framebuffer implementation and fixed-sun orthographic matrix.
#include "render/ShadowMap.h"

#include <stdexcept>

#include <glad/gl.h>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr float kLightDistanceFactor = 1.5f;
constexpr float kOrthoExtentFactor = 0.8f;
constexpr float kFarFactor = 3.0f;
}  // namespace

glm::mat4 shadowLightSpaceMatrix(const glm::vec3& lightDirection,
                                 const glm::vec3& worldCentre,
                                 float worldSize) {
    const glm::vec3 direction = glm::normalize(lightDirection);
    const glm::vec3 sunPosition =
        worldCentre - direction * (worldSize * kLightDistanceFactor);
    const glm::mat4 view =
        glm::lookAt(sunPosition, worldCentre, glm::vec3(0.f, 1.f, 0.f));
    const float extent = worldSize * kOrthoExtentFactor;
    const glm::mat4 projection =
        glm::ortho(-extent, extent, -extent, extent, 0.1f,
                   worldSize * kFarFactor);
    return projection * view;
}

ShadowMap::ShadowMap(int resolution) : size(resolution) {
    glGenFramebuffers(1, &framebuffer);
    glGenTextures(1, &depthTexture);

    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size, size, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[4] = {1.f, 1.f, 1.f, 1.f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &depthTexture);
        framebuffer = 0;
        depthTexture = 0;
        throw std::runtime_error("Shadow-map framebuffer is incomplete");
    }
}

ShadowMap::~ShadowMap() {
    if (depthTexture != 0) {
        glDeleteTextures(1, &depthTexture);
    }
    if (framebuffer != 0) {
        glDeleteFramebuffers(1, &framebuffer);
    }
}

void ShadowMap::beginRender() {
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, size, size);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::endRender() {
    glBindFramebuffer(GL_FRAMEBUFFER,
                      static_cast<GLuint>(previousFramebuffer));
    glViewport(previousViewport[0], previousViewport[1],
               previousViewport[2], previousViewport[3]);
}

unsigned int ShadowMap::texture() const {
    return depthTexture;
}
