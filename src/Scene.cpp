#include "Scene.h"
#include "Primitives.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <sstream>

namespace {
glm::vec3 srgbToLinear(const glm::vec3& c) { return glm::pow(c, glm::vec3(2.2f)); }

// Frame-rate independent approach of value towards target.
float approach(float value, float target, float speed, float dt) {
    return value + (target - value) * (1.0f - std::exp(-speed * dt));
}

const glm::vec3 kWarmLight = glm::pow(glm::vec3(1.0f, 0.72f, 0.42f), glm::vec3(2.2f));
const glm::vec3 kFairyLight = glm::pow(glm::vec3(1.0f, 0.66f, 0.32f), glm::vec3(2.2f));
}  // namespace

Scene::Scene() {
    m_root = std::make_unique<SceneNode>("room");
    buildRoom();
    buildWindow();
    buildDesk();
    buildBed();
    buildShelves();
    buildDecor();
    buildRecordCorner();
    m_root->updateWorld();
}

// --------------------------------------------------------------------------
// resource helpers
// --------------------------------------------------------------------------
const Mesh* Scene::keep(std::unique_ptr<Mesh> m) {
    m_meshes.push_back(std::move(m));
    return m_meshes.back().get();
}

const Mesh* Scene::boxMesh(const glm::vec3& s, float uvPerMeter) {
    std::ostringstream key;
    key << s.x << ',' << s.y << ',' << s.z << ',' << uvPerMeter;
    auto it = m_boxCache.find(key.str());
    if (it != m_boxCache.end()) return it->second;
    const Mesh* m = keep(prim::box(s, uvPerMeter));
    m_boxCache[key.str()] = m;
    return m;
}

const Texture* Scene::tex(const std::string& name, bool srgb) {
    auto it = m_textures.find(name);
    if (it != m_textures.end()) return it->second.get();
    auto t = std::make_unique<Texture>("assets/textures/" + name, srgb);
    const Texture* raw = t.get();
    m_textures[name] = std::move(t);
    return raw;
}

Material Scene::mat(const glm::vec3& srgbColor, const std::string& texture, float specular, float shininess) {
    Material m;
    m.albedo = srgbToLinear(srgbColor);
    m.texture = texture.empty() ? nullptr : tex(texture);
    m.specular = specular;
    m.shininess = shininess;
    return m;
}

SceneNode* Scene::addBox(SceneNode* parent, const std::string& name, const glm::vec3& size,
                         const glm::vec3& pos, const Material& m, float uvPerMeter) {
    return parent->add(name, boxMesh(size, uvPerMeter), m, pos);
}

int Scene::objectCount() const {
    int n = 0;
    m_root->forEach([&](const SceneNode& node) {
        if (node.mesh && !node.shadowOnly) n += node.instanced ? node.mesh->instanceCount() : 1;
    });
    return n;
}

// --------------------------------------------------------------------------
// animation
// --------------------------------------------------------------------------
void Scene::update(float dt, float time, const AppState& st, const SkyState& sky) {
    m_time = time;

    // light switches fade smoothly (bulbs warm up in ~0.3 s)
    m_deskLevel = approach(m_deskLevel, st.deskLamp ? 1.0f : 0.0f, 8.0f, dt);
    m_floorLevel = approach(m_floorLevel, st.floorLamp ? 1.0f : 0.0f, 8.0f, dt);
    m_fairyLevel = approach(m_fairyLevel, st.fairyLights ? 1.0f : 0.0f, 5.0f, dt);

    // wall clock shows the in-game time: hands are children of the clock node,
    // rotating around the clock's local Z axis
    float h12 = std::fmod(sky.hour, 12.0f);
    float minutes = std::fmod(sky.hour, 1.0f) * 60.0f;
    m_hourHand->rotationDeg.z = -h12 / 12.0f * 360.0f - minutes / 60.0f * 30.0f;
    m_minuteHand->rotationDeg.z = -minutes / 60.0f * 360.0f;

    // record player: platter spins at 33 1/3 rpm, tonearm swings onto the record
    float rpm = st.recordPlaying ? 33.333f : 0.0f;
    m_platter->rotationDeg.y = std::fmod(m_platter->rotationDeg.y - rpm * 6.0f * dt, 360.0f);
    m_tonearmAngle = approach(m_tonearmAngle, st.recordPlaying ? 24.0f : 0.0f, 3.0f, dt);
    m_tonearm->rotationDeg.y = m_tonearmAngle;

    // desk lamp joints (user controlled)
    m_lampYaw->rotationDeg.y = st.lampYaw;
    m_lampHead->rotationDeg.z = st.lampHeadPitch;

    // emissive parts follow the light level
    m_lampBulb->material.emissive = kWarmLight * (6.0f * m_deskLevel);
    m_floorBulb->material.emissive = kWarmLight * (6.0f * m_floorLevel);
    m_floorShade->material.emissive = kWarmLight * (0.9f * m_floorLevel);
    m_fairyBulbs->material.emissive = glm::vec3(5.0f * m_fairyLevel);

    m_root->updateWorld();

    // world-space anchors for the point lights
    m_floorLampLightPos = m_floorBulb->worldPosition();
    m_laptopGlowPos = glm::vec3(m_laptopScreen->world * glm::vec4(0, 0, 0.15f, 1));
}

LightSet Scene::lights(const SkyState& sky) const {
    LightSet ls;
    ls.dirLightDir = sky.lightDir;
    ls.dirLightColor = sky.lightColor;
    ls.ambientSky = sky.ambientSky;
    ls.ambientGround = sky.ambientGround;

    // desk lamp spotlight: position and direction come from the lamp head's
    // world matrix, so moving the arm moves the light (hierarchy -> lighting)
    const glm::mat4& head = m_lampHead->world;
    ls.spot.direction = glm::normalize(glm::vec3(head * glm::vec4(0, -1, 0, 0)));
    ls.spot.position = glm::vec3(head * glm::vec4(0, 0, 0, 1)) + ls.spot.direction * 0.04f;
    ls.spot.color = kWarmLight * (5.0f * m_deskLevel);
    // short range: there is no shadow map for the lamp, so the light must fade
    // out before it could leak through the desk onto the floor
    ls.spot.radius = 1.2f;
    ls.spot.innerCos = std::cos(glm::radians(22.0f));
    ls.spot.outerCos = std::cos(glm::radians(36.0f));

    if (m_floorLevel > 0.001f)
        ls.points.push_back({m_floorLampLightPos, kWarmLight * (3.2f * m_floorLevel), 5.5f});

    if (m_fairyLevel > 0.001f) {
        for (size_t i = 0; i < m_fairyPositions.size(); ++i) {
            float twinkle = 0.8f + 0.2f * std::sin(m_time * 2.3f + float(i) * 1.7f);
            ls.points.push_back({m_fairyPositions[i] + glm::vec3(0, 0, 0.08f),
                                 kFairyLight * (0.28f * m_fairyLevel * twinkle), 1.5f});
        }
    }

    // soft blue glow of the laptop screen (always on, matters at night)
    ls.points.push_back({m_laptopGlowPos, glm::vec3(0.10f, 0.14f, 0.28f), 1.4f});

    if (ls.points.size() > size_t(kMaxPointLights)) ls.points.resize(kMaxPointLights);
    return ls;
}
