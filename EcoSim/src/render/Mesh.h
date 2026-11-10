// GPU mesh wrapper: uploads a MeshData into VAO/VBO/EBO, draws it, and owns
// the OpenGL objects. Move-only so buffers are released exactly once.
#pragma once

#include "render/MeshData.h"

// Owns one VAO and its vertex/index buffers.
class Mesh {
public:
    // Uploads positions (attribute 0) and normals (attribute 1).
    explicit Mesh(const MeshData& data);
    // Deletes the VAO and buffers.
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    // Transfers ownership of the GL objects.
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // Draws the indexed triangles with the currently bound program.
    void draw() const;

private:
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    int indexCount = 0;
};
