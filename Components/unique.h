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
struct Viewport
{
    Vector2 boundary;
    float width;
    float height;
    pixel frame_buffer[max_height][max_width]{};
};

class line
{
    // implicitly runs from point A->B
public:
    std::vector<Vector3> endpoints;
    void set_endpoints(Vector3 a, Vector3 b);
    void set_edge_points(Viewport *window);
};
class shape
{
    // assume an input order of left to right
public:
    std::vector<Vector3> endpoints;
    std::vector<line> edges;
    Vector3 colour;
    void set_fiiled_shape(std::vector<Vector3> endpoints, Vector3 colour, Viewport *window);
    void set_unfiiled_shape(std::vector<Vector3> endpoints, Viewport *window);
    void print_framebuffer(Viewport* window);

};

