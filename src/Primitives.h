#pragma once
// Procedural geometry: every object in the room is built from these.
// All generators produce positions, normals and UVs.

#include "Mesh.h"
#include <memory>

namespace prim {

// Axis-aligned box centred at the origin. If uvPerMeter > 0 the UVs are scaled
// by the face size in metres, so a texture keeps the same density on every
// face (no stretching). If 0, each face gets 0..1 UVs.
std::unique_ptr<Mesh> box(const glm::vec3& size, float uvPerMeter = 0.0f);

// Horizontal plane (XZ) centred at the origin, normal +Y.
std::unique_ptr<Mesh> plane(float width, float depth, float uvRepeatX = 1.0f, float uvRepeatZ = 1.0f);

// Vertical quad (XY) centred at the origin, normal +Z, UV 0..1.
std::unique_ptr<Mesh> quad(float width, float height);

std::unique_ptr<Mesh> sphere(float radius, int segments = 32, int rings = 16);

// Cylinder / truncated cone standing on y = 0 up to y = height.
std::unique_ptr<Mesh> cylinder(float radiusBottom, float radiusTop, float height,
                               int segments = 32, bool caps = true);

// Torus in the XY plane (used for the mug handle and the clock rim).
std::unique_ptr<Mesh> torus(float majorRadius, float minorRadius, int segments = 32, int sides = 12);

}  // namespace prim
