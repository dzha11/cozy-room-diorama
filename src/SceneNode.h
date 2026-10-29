#pragma once
// Scene graph node. Each node has a local transform (translate, rotate in
// degrees Y->X->Z, scale) relative to its parent; world matrices are computed
// recursively: world = parentWorld * T * R * S.
// A node may carry a mesh with a material, or just act as a joint/pivot.

#include "Mesh.h"
#include <glm/glm.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Texture;

struct Material {
    glm::vec3 albedo{1.0f};        // linear RGB, multiplied with the texture
    const Texture* texture = nullptr;
    float specular = 0.25f;        // specular strength
    float shininess = 32.0f;       // Blinn-Phong exponent
    glm::vec3 emissive{0.0f};      // linear RGB added after lighting (HDR, can be > 1)
    bool textureIsEmissive = false; // texture also drives emission (screens, signs)
    float alpha = 1.0f;            // < 1 -> drawn in the transparent pass
    float worldUV = 0.0f;          // > 0: planar UVs from world position (repeats per metre),
                                   // keeps walls/floor seamless across separate boxes
};

struct SceneNode {
    std::string name;
    glm::vec3 position{0.0f};
    glm::vec3 rotationDeg{0.0f};   // x = pitch, y = yaw, z = roll
    glm::vec3 scale{1.0f};

    const Mesh* mesh = nullptr;
    Material material;
    bool visible = true;
    bool castShadow = true;
    bool shadowOnly = false;       // invisible occluder (ceiling, missing walls)
    bool instanced = false;        // draw mesh->drawInstanced()

    SceneNode* parent = nullptr;
    std::vector<std::unique_ptr<SceneNode>> children;
    glm::mat4 world{1.0f};

    SceneNode() = default;
    explicit SceneNode(std::string n) : name(std::move(n)) {}

    // Creates a child node and returns a raw pointer to it (owned by this node).
    SceneNode* add(const std::string& childName, const Mesh* m = nullptr,
                   const Material& mat = Material{}, const glm::vec3& pos = glm::vec3(0.0f));

    glm::mat4 localMatrix() const;
    void updateWorld(const glm::mat4& parentWorld = glm::mat4(1.0f));
    void forEach(const std::function<void(const SceneNode&)>& fn) const;
    SceneNode* find(const std::string& n);

    glm::vec3 worldPosition() const { return glm::vec3(world[3]); }
};
