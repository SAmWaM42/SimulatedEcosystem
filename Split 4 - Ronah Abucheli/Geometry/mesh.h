#pragma once

#include <vector>
#include <cstdint>

struct Vec2
{
    float x;
    float y;
};

struct Vec3
{
    float x;
    float y;
    float z;

    Vec3 operator+(const Vec3 &other) const;
    Vec3 operator-(const Vec3 &other) const;
    Vec3 operator*(float scalar) const;
    Vec3 operator/(float scalar) const;
};

struct Transform
{
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
};

struct Mesh
{
    std::vector<Vec3> vertices;
    std::vector<unsigned int> indices;

    void addTriangle(const Vec3 &a, const Vec3 &b, const Vec3 &c);
    void addQuad(const Vec3 &a, const Vec3 &b, const Vec3 &c, const Vec3 &d);
    void addCube(float sideLength);
};

Vec3 applyTransform(const Vec3 &point, const Transform &transform);
