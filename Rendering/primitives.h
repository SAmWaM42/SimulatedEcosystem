#ifndef primitives
#define primitives
#include <vector>
#include <string>
class point{
    float x;
    float y;
    float z;
    point(float x,float y,float z);
};
class shape{
 std::vector<point> vectors;
 std::vector<int> indices;
 void create_shape(std::vector<point> vectors, std::vector<int> indices);
 void create_base_primitive(std::string s_name,std::vector<point> vectors); // function to draw things like triangles using one class
};

#endif