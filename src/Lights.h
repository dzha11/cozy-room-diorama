#pragma once
// Light data sent to the scene shader every frame.

#include <glm/glm.hpp>
#include <vector>

constexpr int kMaxPointLights = 24;  // must match MAX_POINT_LIGHTS in scene.frag

struct PointLight {
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f};   // linear RGB * intensity
    float radius = 3.0f;     // light fades to exactly 0 at this distance
};

struct SpotLight {
    glm::vec3 position{0.0f};
    glm::vec3 direction{0, -1, 0};
    glm::vec3 color{0.0f};
    float radius = 6.0f;
    float innerCos = 0.95f;  // full intensity inside this cone
    float outerCos = 0.80f;  // zero outside this cone
};

struct LightSet {
    glm::vec3 dirLightDir{0, 1, 0};  // towards the sun/moon
    glm::vec3 dirLightColor{1.0f};
    glm::vec3 ambientSky{0.1f};
    glm::vec3 ambientGround{0.05f};
    std::vector<PointLight> points;
    SpotLight spot;
};
