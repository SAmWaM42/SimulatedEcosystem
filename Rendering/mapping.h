#pragma 
#include <primitives.h>
class pixel
{
public:
    float z;
    Vec3 rgb;
    bool valid;
};

struct Viewport
{
    Vec2 boundary;
    float width;
    float height;
    pixel frame_buffer[max_height][max_width]{};
    Scene c_Scene;
    Vec2 viewport_map(Vec2 projected);
};