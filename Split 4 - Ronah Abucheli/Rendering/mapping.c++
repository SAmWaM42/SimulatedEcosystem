#include <mapping.h>
Vec2 viewport_map(Vec2 projected)
{
    return {
        (projected.x + 1.0f) * (max_width - 1) / 2.0f,
        (1.0f - projected.y) * (max_height - 1) / 2.0f
    };
}