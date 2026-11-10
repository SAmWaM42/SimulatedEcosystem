// GL-free procedural mesh generation; all meshes are unit-sized and use
// counter-clockwise winding seen from outside so back-face culling is correct.
#pragma once

#include "render/MeshData.h"

class Terrain;

// 1x1x1 cube centred at the origin with flat per-face normals.
MeshData makeCube();
// Sphere of radius 0.5 with smooth normals; an ellipsoid is this mesh with a
// non-uniform model scale.
MeshData makeSphere(int stacks = 16, int slices = 24);
// Cylinder of radius 0.5 and height 1 (y in [-0.5, 0.5]) with side and caps.
MeshData makeCylinder(int slices = 16);
// Cone with base radius 0.5 at y = -0.5 and apex at y = 0.5.
MeshData makeCone(int slices = 16);
// 1x1 plane in XZ centred at the origin, facing +Y.
MeshData makePlane();
// Terrain mesh with one vertex per grid point and normals sampled from t.
MeshData makeTerrainMesh(const Terrain& t);
