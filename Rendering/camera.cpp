#include "camera.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <cmath>

Camera::Camera()
    : position{0.0f, 0.0f, 5.0f},
      rotation{0.0f, 0.0f, 0.0f},
      fovY(60.0f * static_cast<float>(M_PI) / 180.0f),
      aspectRatio(16.0f / 9.0f),
      nearPlane(0.1f),
      farPlane(100.0f),
      viewMatrix(),
      viewDirty(true)
{
}

void Camera::updateViewMatrix()
{
    Transformer viewTransform;
    viewTransform.rotate(Vec3{-rotation.x, -rotation.y, -rotation.z});
    viewTransform.translate(Vec3{-position.x, -position.y, -position.z});
    viewTransform.set_tranformation_matrix();
    viewMatrix = viewTransform.transformation_matrix;
    viewDirty = false;
}

Vec3 Camera::worldToCamera(const Vec3 &point) const
{
    if (viewDirty)
    {
        const_cast<Camera *>(this)->updateViewMatrix();
    }

    const Vec4 transformed = viewMatrix.muliplyByVec({point.x, point.y, point.z, 1.0f});
    return Vec3{transformed.x, transformed.y, transformed.z};
}
bool Camera::isVisible(const Vec3 &point) const
{
    const Vec3 cameraPoint = worldToCamera(point);
    return cameraPoint.z > nearPlane && cameraPoint.z < farPlane;
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

