// Immediate-mode debug line batch for selection rings and target links; GL
// resources are owned by this object and re-uploaded on every flush.
#pragma once

#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "render/Shader.h"

class Terrain;

// Batches coloured line segments and draws them with its own line shader.
class DebugDraw {
public:
    // Builds the line shader from the assets next to the executable.
    DebugDraw();
    // Deletes the line shader and GL buffers.
    ~DebugDraw();

    DebugDraw(const DebugDraw&) = delete;
    DebugDraw& operator=(const DebugDraw&) = delete;
    DebugDraw(DebugDraw&& other) noexcept;
    DebugDraw& operator=(DebugDraw&& other) noexcept;

    // Queues one world-space line segment.
    void line(glm::vec3 a, glm::vec3 b, glm::vec3 colour);
    // Queues a ring of segments that follows the terrain at height + 2 units.
    void circleOnTerrain(glm::vec2 centre, float radius, glm::vec3 colour,
                         const Terrain& terrain, int segments = 96);
    // Uploads, draws, and clears all queued segments.
    void flush(const glm::mat4& view, const glm::mat4& proj);

private:
    struct LineVertex {
        glm::vec3 position;
        glm::vec3 colour;
    };

    Shader lineShader;
    unsigned int vao = 0;
    unsigned int vbo = 0;
    std::vector<LineVertex> vertices;
};
