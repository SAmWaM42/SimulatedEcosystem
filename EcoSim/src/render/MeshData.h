// Vertex and indexed-triangle data shared by the GL-free mesh factory and the
// GL uploader; no OpenGL types appear here.
#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

// One position and one unit normal.
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

// CPU-side indexed triangle mesh.
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

static_assert(sizeof(Vertex) == 6 * sizeof(float),
              "Vertex must be tightly packed for the GL attribute layout");
