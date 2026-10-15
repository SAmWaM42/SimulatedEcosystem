#pragma once

#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <assert.h>
#define max_width 1280
#define max_height 720
// always rotate->scale->translate
float degree_to_radians(float angle);
float radians_to_degrees(float radians);
bool approximately_equal(float a, float b, float epsilon = 1e-5f);
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
struct Vec4
{
    float x;
    float y;
    float z;
    float w;
};
const Vec4 identity_matrix[4] = {
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f}};
struct Matrix
{
    Vec4 m_values[4]; // default should be a 4*4
    Vec4 muliplyByVec(Vec4 val) const;
    Matrix muliplyBymat(Matrix val) const;
    Matrix inverse_rotation(const Matrix &rotation_matrix) const;
    Matrix();
};
struct Transformer
{
    Matrix rotation_matrix;
    Matrix scale_matrix;
    Matrix translation_matrix;
    Matrix transformation_matrix; // stored for reuse later and can be reset

    void rotate(Vec3 rotation);
    void scale(Vec3 factor);
    void translate(Vec3 offset);
    void set_tranformation_matrix(); // aggregate all the transdormation operations
    Vec3 transform(Vec3 position, float w);
};

// shape primitives
struct obj_Transform
{
    Vec3 rotation;
    Vec3 scale;
    Vec3 position;
};

using Transform = obj_Transform;

class Shape
{
public:
    std::vector<Vec3> vertices;
    std::vector<Vec3> w_vertices;
    obj_Transform transform;

    void world_transform(Transformer t_form); // apparently call each frame so i dont need to check if it has changed
};
