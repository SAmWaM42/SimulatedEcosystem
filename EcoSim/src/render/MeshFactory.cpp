// Mesh factory implementation: unit primitives and terrain grids with outward
// winding, plus a final orientation pass that keeps triangles consistent with
// their vertex normals.
#include "render/MeshFactory.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <glm/geometric.hpp>

#include "world/Terrain.h"

namespace {
constexpr float kPi = 3.14159265358979f;

// Flips any triangle whose geometric normal opposes its vertex normals. This
// makes the CCW winding invariant explicit for generated caps and poles.
void ensureOutwardWinding(MeshData& mesh) {
    for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const Vertex& v0 = mesh.vertices[mesh.indices[i]];
        const Vertex& v1 = mesh.vertices[mesh.indices[i + 1]];
        const Vertex& v2 = mesh.vertices[mesh.indices[i + 2]];
        const glm::vec3 geometricNormal =
            glm::cross(v1.position - v0.position, v2.position - v0.position);
        const glm::vec3 averageNormal =
            glm::normalize(v0.normal + v1.normal + v2.normal);
        if (glm::dot(geometricNormal, averageNormal) < 0.f) {
            std::swap(mesh.indices[i + 1], mesh.indices[i + 2]);
        }
    }
}

// Adds a quad as two triangles; the helper above fixes orientation if needed.
void addQuad(MeshData& mesh, const glm::vec3& normal, const glm::vec3& p0,
             const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
    const std::uint32_t base =
        static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back({p0, normal});
    mesh.vertices.push_back({p1, normal});
    mesh.vertices.push_back({p2, normal});
    mesh.vertices.push_back({p3, normal});
    mesh.indices.insert(mesh.indices.end(),
                        {base, base + 1, base + 2, base, base + 2, base + 3});
}
}  // namespace

MeshData makeCube() {
    MeshData mesh;
    addQuad(mesh, {1.f, 0.f, 0.f}, {0.5f, -0.5f, -0.5f},
            {0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f},
            {0.5f, -0.5f, 0.5f});
    addQuad(mesh, {-1.f, 0.f, 0.f}, {-0.5f, -0.5f, 0.5f},
            {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f},
            {-0.5f, -0.5f, -0.5f});
    addQuad(mesh, {0.f, 1.f, 0.f}, {-0.5f, 0.5f, 0.5f},
            {0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, -0.5f},
            {-0.5f, 0.5f, -0.5f});
    addQuad(mesh, {0.f, -1.f, 0.f}, {-0.5f, -0.5f, -0.5f},
            {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f},
            {-0.5f, -0.5f, 0.5f});
    addQuad(mesh, {0.f, 0.f, 1.f}, {-0.5f, -0.5f, 0.5f},
            {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f},
            {-0.5f, 0.5f, 0.5f});
    addQuad(mesh, {0.f, 0.f, -1.f}, {-0.5f, 0.5f, -0.5f},
            {0.5f, 0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
            {-0.5f, -0.5f, -0.5f});
    ensureOutwardWinding(mesh);
    return mesh;
}

MeshData makeSphere(int stacks, int slices) {
    stacks = std::max(2, stacks);
    slices = std::max(3, slices);

    MeshData mesh;
    const int stride = slices + 1;
    for (int i = 0; i <= stacks; ++i) {
        const float phi = kPi * static_cast<float>(i) /
                          static_cast<float>(stacks);
        const float y = 0.5f * std::cos(phi);
        const float ringRadius = 0.5f * std::sin(phi);
        for (int j = 0; j <= slices; ++j) {
            const float theta = 2.f * kPi * static_cast<float>(j) /
                                static_cast<float>(slices);
            const glm::vec3 position(ringRadius * std::cos(theta), y,
                                     ringRadius * std::sin(theta));
            mesh.vertices.push_back(
                {position, glm::normalize(position)});
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            const std::uint32_t a =
                static_cast<std::uint32_t>(i * stride + j);
            const std::uint32_t b = a + 1;
            const std::uint32_t c =
                static_cast<std::uint32_t>((i + 1) * stride + j);
            const std::uint32_t d = c + 1;
            if (i == 0) {
                mesh.indices.insert(mesh.indices.end(), {a, d, c});
            } else if (i == stacks - 1) {
                mesh.indices.insert(mesh.indices.end(), {a, b, c});
            } else {
                mesh.indices.insert(mesh.indices.end(),
                                    {a, b, d, a, d, c});
            }
        }
    }

    ensureOutwardWinding(mesh);
    return mesh;
}

MeshData makeCylinder(int slices) {
    slices = std::max(3, slices);
    MeshData mesh;

    for (int j = 0; j <= slices; ++j) {
        const float theta = 2.f * kPi * static_cast<float>(j) /
                            static_cast<float>(slices);
        const float nx = std::cos(theta);
        const float nz = std::sin(theta);
        const glm::vec3 normal(nx, 0.f, nz);
        mesh.vertices.push_back({{0.5f * nx, -0.5f, 0.5f * nz}, normal});
        mesh.vertices.push_back({{0.5f * nx, 0.5f, 0.5f * nz}, normal});
    }

    for (int j = 0; j < slices; ++j) {
        const std::uint32_t bottom =
            static_cast<std::uint32_t>(2 * j);
        const std::uint32_t top = bottom + 1;
        const std::uint32_t nextBottom = bottom + 2;
        const std::uint32_t nextTop = bottom + 3;
        mesh.indices.insert(mesh.indices.end(),
                            {bottom, nextBottom, nextTop, bottom, nextTop, top});
    }

    // Top cap with its own +Y normals.
    std::uint32_t center =
        static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back({{0.f, 0.5f, 0.f}, {0.f, 1.f, 0.f}});
    const std::uint32_t topRing =
        static_cast<std::uint32_t>(mesh.vertices.size());
    for (int j = 0; j <= slices; ++j) {
        const float theta = 2.f * kPi * static_cast<float>(j) /
                            static_cast<float>(slices);
        mesh.vertices.push_back(
            {{0.5f * std::cos(theta), 0.5f, 0.5f * std::sin(theta)},
             {0.f, 1.f, 0.f}});
    }
    for (int j = 0; j < slices; ++j) {
        mesh.indices.insert(mesh.indices.end(),
                            {center, topRing + static_cast<std::uint32_t>(j),
                             topRing + static_cast<std::uint32_t>(j + 1)});
    }

    // Bottom cap with its own -Y normals.
    center = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back({{0.f, -0.5f, 0.f}, {0.f, -1.f, 0.f}});
    const std::uint32_t bottomRing =
        static_cast<std::uint32_t>(mesh.vertices.size());
    for (int j = 0; j <= slices; ++j) {
        const float theta = 2.f * kPi * static_cast<float>(j) /
                            static_cast<float>(slices);
        mesh.vertices.push_back(
            {{0.5f * std::cos(theta), -0.5f, 0.5f * std::sin(theta)},
             {0.f, -1.f, 0.f}});
    }
    for (int j = 0; j < slices; ++j) {
        mesh.indices.insert(
            mesh.indices.end(),
            {center, bottomRing + static_cast<std::uint32_t>(j + 1),
             bottomRing + static_cast<std::uint32_t>(j)});
    }

    ensureOutwardWinding(mesh);
    return mesh;
}

MeshData makeCone(int slices) {
    slices = std::max(3, slices);
    MeshData mesh;

    const std::uint32_t apex =
        static_cast<std::uint32_t>(slices + 1);
    for (int j = 0; j <= slices; ++j) {
        const float theta = 2.f * kPi * static_cast<float>(j) /
                            static_cast<float>(slices);
        const float nx = std::cos(theta);
        const float nz = std::sin(theta);
        mesh.vertices.push_back(
            {{0.5f * nx, -0.5f, 0.5f * nz},
             glm::normalize(glm::vec3(nx, 0.5f, nz))});
    }
    mesh.vertices.push_back({{0.f, 0.5f, 0.f}, {0.f, 1.f, 0.f}});

    for (int j = 0; j < slices; ++j) {
        mesh.indices.insert(mesh.indices.end(),
                            {static_cast<std::uint32_t>(j),
                             static_cast<std::uint32_t>(j + 1), apex});
    }

    // Base cap with downward normals and its own ring vertices.
    const std::uint32_t center =
        static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back({{0.f, -0.5f, 0.f}, {0.f, -1.f, 0.f}});
    const std::uint32_t baseRing =
        static_cast<std::uint32_t>(mesh.vertices.size());
    for (int j = 0; j <= slices; ++j) {
        const float theta = 2.f * kPi * static_cast<float>(j) /
                            static_cast<float>(slices);
        mesh.vertices.push_back(
            {{0.5f * std::cos(theta), -0.5f, 0.5f * std::sin(theta)},
             {0.f, -1.f, 0.f}});
    }
    for (int j = 0; j < slices; ++j) {
        mesh.indices.insert(
            mesh.indices.end(),
            {center, baseRing + static_cast<std::uint32_t>(j + 1),
             baseRing + static_cast<std::uint32_t>(j)});
    }

    ensureOutwardWinding(mesh);
    return mesh;
}

MeshData makePlane() {
    MeshData mesh;
    const glm::vec3 normal(0.f, 1.f, 0.f);
    mesh.vertices.push_back({{-0.5f, 0.f, -0.5f}, normal});
    mesh.vertices.push_back({{-0.5f, 0.f, 0.5f}, normal});
    mesh.vertices.push_back({{0.5f, 0.f, 0.5f}, normal});
    mesh.vertices.push_back({{0.5f, 0.f, -0.5f}, normal});
    mesh.indices = {0, 1, 2, 0, 2, 3};
    ensureOutwardWinding(mesh);
    return mesh;
}

MeshData makeTerrainMesh(const Terrain& t) {
    MeshData mesh;
    const int res = t.resolution();
    if (res < 2) {
        return mesh;
    }

    const TerrainConfig& cfg = t.config();
    const float denominator = static_cast<float>(res - 1);
    mesh.vertices.reserve(
        static_cast<std::size_t>(res) * static_cast<std::size_t>(res));
    for (int iz = 0; iz < res; ++iz) {
        for (int ix = 0; ix < res; ++ix) {
            const float x = cfg.size * static_cast<float>(ix) / denominator;
            const float z = cfg.size * static_cast<float>(iz) / denominator;
            mesh.vertices.push_back(
                {{x, t.vertexHeight(ix, iz), z}, t.normalAt(x, z)});
        }
    }

    for (int iz = 0; iz < res - 1; ++iz) {
        for (int ix = 0; ix < res - 1; ++ix) {
            const std::uint32_t v00 =
                static_cast<std::uint32_t>(iz * res + ix);
            const std::uint32_t v10 = v00 + 1;
            const std::uint32_t v01 =
                static_cast<std::uint32_t>((iz + 1) * res + ix);
            const std::uint32_t v11 = v01 + 1;
            mesh.indices.insert(mesh.indices.end(),
                                {v00, v01, v11, v00, v11, v10});
        }
    }

    ensureOutwardWinding(mesh);
    return mesh;
}
