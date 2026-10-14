#include "scene_manager.h"

void SceneManager::addCube(const Vec3 &position, float sideLength, const Vec3 &color)
{
    SceneObject object;
    object.mesh.addCube(sideLength);
    object.transform.position = position;
    object.transform.rotation = Vec3{0.0f, 0.0f, 0.0f};
    object.transform.scale = Vec3{1.0f, 1.0f, 1.0f};
    object.color = color;
    objects.push_back(object);
}

void SceneManager::addMesh(const Mesh &mesh, const Transform &transform, const Vec3 &color)
{
    SceneObject object;
    object.mesh = mesh;
    object.transform = transform;
    object.color = color;
    objects.push_back(object);
}

const std::vector<SceneObject> &SceneManager::getObjects() const
{
    return objects;
}
