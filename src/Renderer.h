#pragma once
// Renderer: runs the frame.
//   shadow pass -> scene pass (opaque, sky backdrop, dust, transparent glass)
//   -> MSAA resolve -> bloom + light shafts -> tone-mapped composite

#include "AppState.h"
#include "Camera.h"
#include "DayCycle.h"
#include "DustParticles.h"
#include "Lights.h"
#include "PostProcess.h"
#include "Scene.h"
#include "Shader.h"
#include "ShadowMap.h"

#include <memory>

struct FrameStats {
    int drawCalls = 0;
    int pointLights = 0;
};

class Renderer {
public:
    Renderer();
    void render(const Scene& scene, const Camera& cam, const AppState& st, const SkyState& sky,
                const LightSet& lights, int fbWidth, int fbHeight, float time);
    const FrameStats& stats() const { return m_stats; }
    int dustCount() const { return m_dust->count(); }

private:
    void shadowPass(const Scene& scene);
    void drawNodes(const Scene& scene, const Shader& sh, bool transparentPass);
    void setMaterial(const Shader& sh, const Material& m) const;
    void uploadLights(const LightSet& lights) const;
    void drawSky(const Scene& scene, const Camera& cam, const SkyState& sky, const glm::mat4& view,
                 const glm::mat4& proj, float time);

    std::unique_ptr<Shader> m_sceneShader, m_shadowShader, m_skyShader;
    std::unique_ptr<ShadowMap> m_shadowMap;
    std::unique_ptr<PostProcess> m_post;
    std::unique_ptr<DustParticles> m_dust;
    std::unique_ptr<Mesh> m_skyQuad;
    mutable FrameStats m_stats;
};
