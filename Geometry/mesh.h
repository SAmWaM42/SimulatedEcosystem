#pragma once

#include <vector>
#include <cstdint>
#include <primitives.h>
struct Mesh
{
    std::vector<Vec3> vertices;
    std::vector<unsigned int> indices;

    void addTriangle(const Vec3 &a, const Vec3 &b, const Vec3 &c);
    void addQuad(const Vec3 &a, const Vec3 &b, const Vec3 &c, const Vec3 &d);
    void addCube(float sideLength);
};

Vec3 applyTransform(const Vec3 &point, const Transform &transform);
