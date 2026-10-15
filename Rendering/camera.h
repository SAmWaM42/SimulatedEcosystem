#pragma once

#include "../Geometry/mesh.h"

struct Camera
{
    Vec3 position;
    Vec3 rotation;
    float fovY;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    Matrix viewMatrix;
    bool viewDirty;

    Camera();
    void updateViewMatrix();
    Vec3 worldToCamera(const Vec3 &point) const;
    Vec3 projectToScreen(const Vec3 &point, int width, int height) const;
    bool isVisible(const Vec3 &point) const;
};
