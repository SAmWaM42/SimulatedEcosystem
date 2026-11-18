#include "mesh.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <cmath>

namespace
{
Vec3 rotatePoint(const Vec3 &point, const Vec3 &rotation)
{
    const float rx = rotation.x * static_cast<float>(M_PI) / 180.0f;
    const float ry = rotation.y * static_cast<float>(M_PI) / 180.0f;
    const float rz = rotation.z * static_cast<float>(M_PI) / 180.0f;

    const float sx = std::sin(rx);
    const float cx = std::cos(rx);
    const float sy = std::sin(ry);
    const float cy = std::cos(ry);
    const float sz = std::sin(rz);
    const float cz = std::cos(rz);

    Vec3 p;
    p.x = point.x * (cy * cz) + point.y * (sx * sy * cz - cx * sz) + point.z * (cx * sy * cz + sx * sz);
    p.y = point.x * (cy * sz) + point.y * (sx * sy * sz + cx * cz) + point.z * (cx * sy * sz - sx * cz);
    p.z = point.x * (-sy) + point.y * (sx * cy) + point.z * (cx * cy);
    return p;
}
} // namespace

Vec3 Vec3::operator+(const Vec3 &other) const
{
    return Vec3{x + other.x, y + other.y, z + other.z};
}

Vec3 Vec3::operator-(const Vec3 &other) const
{
    return Vec3{x - other.x, y - other.y, z - other.z};
}

Vec3 Vec3::operator*(float scalar) const
{
    return Vec3{x * scalar, y * scalar, z * scalar};
}

Vec3 Vec3::operator/(float scalar) const
{
    return Vec3{x / scalar, y / scalar, z / scalar};
}

Vec3 applyTransform(const Vec3 &point, const Transform &transform)
{
    Vec3 scaled = point;
    scaled.x *= transform.scale.x;
    scaled.y *= transform.scale.y;
    scaled.z *= transform.scale.z;

    Vec3 rotated = rotatePoint(scaled, transform.rotation);
    return Vec3{rotated.x + transform.position.x, rotated.y + transform.position.y, rotated.z + transform.position.z};
}

void Mesh::addTriangle(const Vec3 &a, const Vec3 &b, const Vec3 &c)
{
    const unsigned int base = static_cast<unsigned int>(vertices.size());
    vertices.push_back(a);
    vertices.push_back(b);
    vertices.push_back(c);
    indices.push_back(base);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
}

void Mesh::addQuad(const Vec3 &a, const Vec3 &b, const Vec3 &c, const Vec3 &d)
{
    addTriangle(a, b, c);
    addTriangle(a, c, d);
}

void Mesh::addCube(float sideLength)
{
    const float h = sideLength * 0.5f;
    const Vec3 p0{-h, -h, -h};
    const Vec3 p1{h, -h, -h};
    const Vec3 p2{h, h, -h};
    const Vec3 p3{-h, h, -h};
    const Vec3 p4{-h, -h, h};
    const Vec3 p5{h, -h, h};
    const Vec3 p6{h, h, h};
    const Vec3 p7{-h, h, h};

    addQuad(p0, p1, p2, p3);
    addQuad(p4, p5, p6, p7);
    addQuad(p0, p4, p7, p3);
    addQuad(p1, p5, p6, p2);
    addQuad(p0, p1, p5, p4);
    addQuad(p3, p2, p6, p7);
}
