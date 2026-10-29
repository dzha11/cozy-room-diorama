#include "Renderer.h"
#include "Primitives.h"
#include "Texture.h"

#include <glm/gtc/matrix_transform.hpp>
#include <string>

Renderer::Renderer() {
    m_sceneShader = std::make_unique<Shader>("scene.vert", "scene.frag");
    m_shadowShader = std::make_unique<Shader>("shadow.vert", "shadow.frag");
    m_skyShader = std::make_unique<Shader>("sky.vert", "sky.frag");
    m_shadowMap = std::make_unique<ShadowMap>(2048);
    m_post = std::make_unique<PostProcess>();
    m_dust = std::make_unique<DustParticles>(6000);
    m_skyQuad = prim::quad(3.6f, 2.35f);
}

void Renderer::setMaterial(const Shader& sh, const Material& m) const {
    sh.set("uAlbedo", m.albedo);
    sh.set("uHasTexture", m.texture != nullptr);
    if (m.texture) m.texture->bind(0);
    sh.set("uSpecular", m.specular);
    sh.set("uShininess", m.shininess);
    sh.set("uEmissive", m.emissive);
    sh.set("uTextureIsEmissive", m.textureIsEmissive);
    sh.set("uAlpha", m.alpha);
    sh.set("uWorldUV", m.worldUV);
}

void Renderer::uploadLights(const LightSet& L) const {
    const Shader& sh = *m_sceneShader;
    sh.set("uDirLightDir", L.dirLightDir);
    sh.set("uDirLightColor", L.dirLightColor);
    sh.set("uAmbientSky", L.ambientSky);
    sh.set("uAmbientGround", L.ambientGround);
    int n = static_cast<int>(L.points.size());
    sh.set("uNumPointLights", n);
    for (int i = 0; i < n; ++i) {
        std::string b = "uPointLights[" + std::to_string(i) + "].";
        sh.set(b + "position", L.points[i].position);
        sh.set(b + "color", L.points[i].color);
        sh.set(b + "radius", L.points[i].radius);
    }
    sh.set("uSpot.position", L.spot.position);
    sh.set("uSpot.direction", L.spot.direction);
    sh.set("uSpot.color", L.spot.color);
    sh.set("uSpot.radius", L.spot.radius);
    sh.set("uSpot.innerCos", L.spot.innerCos);
    sh.set("uSpot.outerCos", L.spot.outerCos);
    m_stats.pointLights = n;
}

void Renderer::shadowPass(const Scene& scene) {
    m_shadowMap->begin();
    m_shadowShader->use();
    m_shadowShader->set("uLightSpace", m_shadowMap->lightSpace());
    scene.root().forEach([&](const SceneNode& n) {
        if (!n.mesh || !n.castShadow || n.material.alpha < 1.0f) return;
        m_shadowShader->set("uInstanced", n.instanced);
        m_shadowShader->set("uModel", n.world);
        if (n.instanced) n.mesh->drawInstanced(); else n.mesh->draw();
        ++m_stats.drawCalls;
    });
    m_shadowMap->end();
}

void Renderer::drawNodes(const Scene& scene, const Shader& sh, bool transparentPass) {
    scene.root().forEach([&](const SceneNode& n) {
        if (!n.mesh || n.shadowOnly) return;
        if ((n.material.alpha < 1.0f) != transparentPass) return;
        setMaterial(sh, n.material);
        sh.set("uInstanced", n.instanced);
        sh.set("uModel", n.world);
        sh.set("uNormalMat", glm::mat3(glm::transpose(glm::inverse(n.world))));
        if (n.instanced) n.mesh->drawInstanced(); else n.mesh->draw();
        ++m_stats.drawCalls;
    });
}

void Renderer::drawSky(const Scene& scene, const Camera& cam, const SkyState& sky, const glm::mat4& view,
                       const glm::mat4& proj, float time) {
    const Shader& s = *m_skyShader;
    s.use();
    glm::mat4 model = glm::translate(glm::mat4(1.0f), scene.windowCenter() + glm::vec3(0, 0.025f, -0.22f));
    s.set("uModel", model);
    s.set("uView", view);
    s.set("uProj", proj);
    s.set("uCameraPos", cam.position());
    s.set("uZenith", sky.zenith);
    s.set("uHorizon", sky.horizon);
    // the sky in the window is a compressed "painting": squeeze azimuth and
    // elevation so the sun is actually visible in the window at dawn and dusk
    s.set("uSunAzimuth", glm::radians(sky.sunAzimuthDeg) * 0.32f);
    s.set("uSunElevation", glm::radians(sky.sunElevationDeg) * 0.7f);
    s.set("uSunVisible", sky.sunVisible);
    float moonEl = glm::degrees(std::asin(sky.moonDir.y));
    float moonAz = glm::degrees(std::atan2(sky.moonDir.x, -sky.moonDir.z));
    s.set("uMoonAzimuth", glm::radians(moonAz) * 0.32f);
    s.set("uMoonElevation", glm::radians(moonEl) * 0.7f);
    s.set("uNight", sky.night);
    s.set("uSunColor", glm::normalize(sky.lightIsSun ? sky.lightColor + glm::vec3(1e-4f) : glm::vec3(1.0f, 0.6f, 0.3f)) * 1.7f);
    s.set("uTime", time);
    m_skyQuad->draw();
    ++m_stats.drawCalls;
}

void Renderer::render(const Scene& scene, const Camera& cam, const AppState& st, const SkyState& sky,
                      const LightSet& lights, int fbW, int fbH, float time) {
    m_stats = FrameStats{};
    if (fbW <= 0 || fbH <= 0) return;
    m_post->resize(fbW, fbH);

    const RoomBounds& rb = scene.bounds();
    // shadow camera covers the whole diorama including the walls
    m_shadowMap->fit(sky.lightDir, glm::vec3(-4.5f, -0.4f, -4.4f), glm::vec3(4.5f, 3.5f, 3.4f));
    bool needShadowMap = st.shadows || st.dust || st.lightShafts;
    if (needShadowMap) shadowPass(scene);

    glm::mat4 view = cam.view();
    glm::mat4 proj = cam.projection(float(fbW) / float(fbH));

    // ---- scene pass into the MSAA HDR buffer ----
    m_post->beginScene();
    glm::vec3 bg = glm::mix(glm::vec3(0.075f, 0.062f, 0.060f), glm::vec3(0.012f, 0.012f, 0.025f), sky.night);
    glClearColor(bg.r, bg.g, bg.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);   // open meshes (lamp shades) must show both sides
    if (st.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    const Shader& sh = *m_sceneShader;
    sh.use();
    sh.set("uView", view);
    sh.set("uProj", proj);
    sh.set("uLightSpace", m_shadowMap->lightSpace());
    sh.set("uCameraPos", cam.position());
    sh.set("uTime", time);
    sh.set("uTexture", 0);
    sh.set("uShadowMap", 4);
    sh.set("uShadowsEnabled", st.shadows);
    sh.set("uShadowTexel", glm::vec2(1.0f / m_shadowMap->size()));
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, m_shadowMap->depthTexture());
    glActiveTexture(GL_TEXTURE0);
    uploadLights(lights);
    drawNodes(scene, sh, false);

    drawSky(scene, cam, sky, view, proj, time);

    if (st.dust) {
        DustParticles::DrawParams p;
        p.view = view;
        p.proj = proj;
        p.lightSpace = m_shadowMap->lightSpace();
        p.boxMin = glm::vec3(-3.9f, 0.05f, -3.95f);
        p.boxMax = glm::vec3(4.1f, 3.1f, 0.6f);
        p.lightColor = sky.lightColor;
        p.ambient = sky.ambientSky;
        p.time = time;
        p.amount = st.dustAmount;
        p.shadowMap = m_shadowMap->depthTexture();
        p.shadowsEnabled = true;   // dust is always masked by the beam
        m_dust->draw(p);
        ++m_stats.drawCalls;
    }

    // transparent glass last, blended over everything behind it
    sh.use();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    drawNodes(scene, sh, true);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    if (st.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    m_post->endScene();

    // ---- post-processing ----
    PostSettings ps;
    ps.bloom = st.bloom;
    ps.shafts = st.lightShafts;
    ps.exposure = st.exposure;
    ps.grade = sky.grade;
    ps.shaftColor = sky.lightColor * 0.45f;
    ps.invViewProj = glm::inverse(proj * view);
    ps.lightSpace = m_shadowMap->lightSpace();
    ps.cameraPos = cam.position();
    ps.lightDir = sky.lightDir;
    ps.boxMin = rb.min;
    ps.boxMax = rb.max;
    ps.shadowMap = m_shadowMap->depthTexture();
    ps.time = time;
    m_post->apply(ps, fbW, fbH);
    m_stats.drawCalls += 4;
}
