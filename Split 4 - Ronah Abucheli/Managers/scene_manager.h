#pragma once

#include <vector>

#include "../Geometry/mesh.h"
#include "../Rendering/camera.h"

struct SceneObject
{
    Mesh mesh;
    Transform transform;
    Vec3 color;
};

class SceneManager
{
public:
    std::vector<SceneObject> objects;
    Camera camera;

    void addCube(const Vec3 &position, float sideLength, const Vec3 &color);
    void addMesh(const Mesh &mesh, const Transform &transform, const Vec3 &color);
    const std::vector<SceneObject> &getObjects() const;
};
