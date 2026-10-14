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
    {0.0f, 0.0f, 0.0f, 1.0f}
};
struct Matrix
{
    Vec4 m_values[4]; // default should be a 4*4
    Vec4 muliplyByVec(Vec4 val);
    Matrix muliplyBymat(Matrix val);
    Matrix inverse_rotation(const Matrix& rotation_matrix);
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
    void set_tranformation_matrix();// aggregate all the transdormation operations
    Vec3 transform(Vec3 position,float w);
};


// shape primitives
struct  obj_Transform
{
    Vec3 rotation;
    Vec3 scale;
    Vec3 position;
  
};

class Shape{
    public:
 std::vector<Vec3> vertices;
 std::vector<Vec3> w_vertices;
 obj_Transform transform;
 
 void world_transform(Transformer t_form); // apparently call each frame so i dont need to check if it has changed
};
class Camera {
public:
    Vec3 position;
    Vec3 rotation;

    Matrix view_matrix;

    void update_view_matrix();
    Vec3 world_to_camera(Vec3 point);
};
class Projection {
public:
    float fov = 90.0f;
    float aspect_ratio = 1280.0f / 720.0f;
    float near_plane = 0.1f;
    float far_plane = 1000.0f;
    bool project_point(Vec3 camera_point, Vec2& projected);
    
};

class Scene{
    public:
 std::vector<std::unique_ptr<Shape>> shapes;
 Camera cam;
 void add_shape(std::unique_ptr<Shape> n_Shape);
 void remove_shape(Shape* n_Shape);

};

