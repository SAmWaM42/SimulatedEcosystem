// Debug line batching implementation.
#include "render/DebugDraw.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

#include <glad/gl.h>

#include "render/Assets.h"
#include "world/Terrain.h"

namespace {
constexpr float kPi = 3.14159265358979f;
}

DebugDraw::DebugDraw()
    : lineShader(assetPath("shaders/line.vert"),
                 assetPath("shaders/line.frag")) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(LineVertex)),
        reinterpret_cast<const void*>(offsetof(LineVertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(LineVertex)),
        reinterpret_cast<const void*>(offsetof(LineVertex, colour)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

DebugDraw::~DebugDraw() {
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
    }
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }
}

DebugDraw::DebugDraw(DebugDraw&& other) noexcept
    : lineShader(std::move(other.lineShader)),
      vao(other.vao),
      vbo(other.vbo),
      vertices(std::move(other.vertices)) {
    other.vao = 0;
    other.vbo = 0;
}

DebugDraw& DebugDraw::operator=(DebugDraw&& other) noexcept {
    if (this != &other) {
        lineShader = std::move(other.lineShader);
        if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
        }
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
        }
        vao = other.vao;
        vbo = other.vbo;
        vertices = std::move(other.vertices);
        other.vao = 0;
        other.vbo = 0;
    }
    return *this;
}

void DebugDraw::line(glm::vec3 a, glm::vec3 b, glm::vec3 colour) {
    vertices.push_back({a, colour});
    vertices.push_back({b, colour});
}

void DebugDraw::circleOnTerrain(glm::vec2 centre, float radius,
                                glm::vec3 colour, const Terrain& terrain,
                                int segments) {
    segments = std::max(3, segments);
    for (int i = 0; i < segments; ++i) {
        const float angle0 =
            2.f * kPi * static_cast<float>(i) / static_cast<float>(segments);
        const float angle1 = 2.f * kPi * static_cast<float>(i + 1) /
                             static_cast<float>(segments);
        const glm::vec2 p0 = centre +
                             glm::vec2(std::cos(angle0), std::sin(angle0)) *
                                 radius;
        const glm::vec2 p1 = centre +
                             glm::vec2(std::cos(angle1), std::sin(angle1)) *
                                 radius;
        line(glm::vec3(p0.x, terrain.heightAt(p0.x, p0.y) + 2.f, p0.y),
             glm::vec3(p1.x, terrain.heightAt(p1.x, p1.y) + 2.f, p1.y),
             colour);
    }
}

void DebugDraw::flush(const glm::mat4& view, const glm::mat4& proj) {
    if (vertices.empty()) {
        return;
    }

    lineShader.use();
    lineShader.setMat4("uViewProj", proj * view);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() *
                                         sizeof(LineVertex)),
                 vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    vertices.clear();
}

