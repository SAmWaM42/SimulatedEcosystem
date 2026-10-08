#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include <map>
#include <algorithm>
#define max_width 1280
#define max_height 920
struct Vector2
{
    float x;
    float y;
};
struct Vector3
{
    float x;
    float y;
    float z;
};
class pixel
{
public:
    float z;
    Vector3 rgb;
    bool valid;
};
struct Transform
{
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
};
class Transformer{
    public:
    Vector3 rotate(Vector3 points,Vector3 rotation);
    Vector3 rotate_inverse(Vector3 points,Vector3 rotation);
    Vector3 scale(Vector3 points,Vector3 scale);
    Vector3 translate(Vector3 points,Vector3 position);
    Vector3 transform(Vector3 points,Transform  transform); 

};
class Camera{
    public:
    Transform transform;
    void view_transform(Transformer trans,std::vector<Vector3>* points);
};
struct Viewport
{
    Vector2 boundary;
    float width;
    float height;
    pixel frame_buffer[max_height][max_width]{};
    Camera cam;
};
class line
{ // implicitly runs from point A->B
public:
    std::vector<Vector3> endpoints;
    void set_endpoints(Vector3 a, Vector3 b);
    void set_edge_points(Viewport *window,Transformer trans,std::vector<Vector2>* screen);
};
class shape
{  // assume an input order of left to right
public:
    std::vector<Vector3> endpoints;
    std::vector<Vector3> relative_endpoints;
    std::vector<Vector2> screen_coords;
    std::vector<line> edges;
    Vector3 colour;
    Transform transform;
    shape(std::vector<Vector3> points,Transform transform,Transformer trans);
    void transform_to_world(Transformer trans);
    void set_fiiled_shape( Vector3 colour, Viewport *window,Transformer trans);
    void set_unfiiled_shape(Viewport *window,Transformer trans);
    void print_framebuffer(Viewport *window);

};

