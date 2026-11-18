#include "camera.h"

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

Camera::Camera()
    : position{0.0f, 0.0f, 5.0f},
      rotation{0.0f, 0.0f, 0.0f},
      fovY(60.0f * static_cast<float>(M_PI) / 180.0f),
      aspectRatio(16.0f / 9.0f),
      nearPlane(0.1f),
      farPlane(100.0f)
{
}

Vec3 Camera::worldToCamera(const Vec3 &point) const
{
    Vec3 relative = point - position;
    const Vec3 rotated = rotatePoint(relative, Vec3{-rotation.x, -rotation.y, -rotation.z});
    return rotated;
}

Vec3 Camera::projectToScreen(const Vec3 &point, int width, int height) const
{
    const Vec3 cameraPoint = worldToCamera(point);

    if (cameraPoint.z <= nearPlane || cameraPoint.z >= farPlane)
    {
        return Vec3{-9999.0f, -9999.0f, cameraPoint.z};
    }

    const float f = 1.0f / std::tan(fovY * 0.5f);
    const float ndcX = (cameraPoint.x / cameraPoint.z) * f * aspectRatio;
    const float ndcY = (cameraPoint.y / cameraPoint.z) * f;

    Vec3 screen;
    screen.x = (ndcX * 0.5f + 0.5f) * static_cast<float>(width - 1);
    screen.y = (1.0f - (ndcY * 0.5f + 0.5f)) * static_cast<float>(height - 1);
    screen.z = cameraPoint.z;
    return screen;
}

bool Camera::isVisible(const Vec3 &point) const
{
    const Vec3 cameraPoint = worldToCamera(point);
    return cameraPoint.z > nearPlane && cameraPoint.z < farPlane;
}
