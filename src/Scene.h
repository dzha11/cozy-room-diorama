#pragma once
// Scene: owns every mesh, texture and the scene graph of the room, animates
// the hierarchical props (clock hands, desk lamp, record player) and produces
// the list of lights for the current frame.

#include "AppState.h"
#include "DayCycle.h"
#include "Lights.h"
#include "Mesh.h"
#include "SceneNode.h"
#include "Texture.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

// Axis-aligned bounds of the room interior (used by light shafts and dust).
struct RoomBounds {
    glm::vec3 min{-4.0f, 0.0f, -4.0f};
    glm::vec3 max{4.0f, 3.2f, 3.2f};
};

class Scene {
public:
    Scene();

    void update(float dt, float time, const AppState& state, const SkyState& sky);
    LightSet lights(const SkyState& sky) const;

    const SceneNode& root() const { return *m_root; }
    const RoomBounds& bounds() const { return m_bounds; }

    // Window opening (world space) - the sky backdrop is drawn behind it.
    glm::vec3 windowCenter() const { return {0.4f, 1.75f, -4.1f}; }
    glm::vec2 windowSize() const { return {2.0f, 1.6f}; }

    float deskLampLevel() const { return m_deskLevel; }
    float floorLampLevel() const { return m_floorLevel; }
    float fairyLevel() const { return m_fairyLevel; }

    int objectCount() const;

private:
    // construction helpers (SceneLayout.cpp)
    void buildRoom();
    void buildWindow();
    void buildDesk();
    void buildBed();
    void buildShelves();
    void buildDecor();
    void buildRecordCorner();

    const Mesh* boxMesh(const glm::vec3& size, float uvPerMeter = 0.0f);
    const Mesh* keep(std::unique_ptr<Mesh> m);
    const Texture* tex(const std::string& name, bool srgb = true);
    Material mat(const glm::vec3& srgbColor, const std::string& texture = "",
                 float specular = 0.2f, float shininess = 24.0f);
    SceneNode* addBox(SceneNode* parent, const std::string& name, const glm::vec3& size,
                      const glm::vec3& pos, const Material& m, float uvPerMeter = 0.0f);

    std::unique_ptr<SceneNode> m_root;
    std::vector<std::unique_ptr<Mesh>> m_meshes;
    std::map<std::string, const Mesh*> m_boxCache;
    std::map<std::string, std::unique_ptr<Texture>> m_textures;
    RoomBounds m_bounds;

    // animated / switchable nodes
    SceneNode* m_hourHand = nullptr;
    SceneNode* m_minuteHand = nullptr;
    SceneNode* m_platter = nullptr;
    SceneNode* m_tonearm = nullptr;
    SceneNode* m_lampYaw = nullptr;
    SceneNode* m_lampHead = nullptr;
    SceneNode* m_lampBulb = nullptr;
    SceneNode* m_floorShade = nullptr;
    SceneNode* m_floorBulb = nullptr;
    SceneNode* m_fairyBulbs = nullptr;
    SceneNode* m_laptopScreen = nullptr;

    std::vector<glm::vec3> m_fairyPositions;
    glm::vec3 m_floorLampLightPos{0.0f};
    glm::vec3 m_laptopGlowPos{0.0f};

    // current light levels 0..1 (fade towards the on/off target)
    float m_deskLevel = 0.0f, m_floorLevel = 0.0f, m_fairyLevel = 0.0f;
    float m_time = 0.0f;
    float m_tonearmAngle = 0.0f;
};
