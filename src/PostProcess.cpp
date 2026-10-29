#include "PostProcess.h"

#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

namespace {
GLuint makeTexture(int w, int h, GLenum internal, GLenum format, GLenum type, GLenum filter) {
    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0, format, type, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return t;
}

GLuint makeColorFbo(GLuint tex) {
    GLuint f;
    glGenFramebuffers(1, &f);
    glBindFramebuffer(GL_FRAMEBUFFER, f);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Post-process framebuffer incomplete");
    return f;
}
}  // namespace

PostProcess::PostProcess() {
    m_bright = std::make_unique<Shader>("fullscreen.vert", "bright.frag");
    m_blur = std::make_unique<Shader>("fullscreen.vert", "blur.frag");
    m_shafts = std::make_unique<Shader>("fullscreen.vert", "shafts.frag");
    m_composite = std::make_unique<Shader>("fullscreen.vert", "composite.frag");
    glGenVertexArrays(1, &m_emptyVao);   // core profile needs a VAO bound to draw
}

PostProcess::~PostProcess() {
    destroy();
    glDeleteVertexArrays(1, &m_emptyVao);
}

void PostProcess::destroy() {
    GLuint fbos[] = {m_msFbo, m_resolveFbo, m_halfFbo[0], m_halfFbo[1], m_shaftFbo};
    GLuint texs[] = {m_hdrTex, m_depthTex, m_halfTex[0], m_halfTex[1], m_shaftTex};
    GLuint rbs[] = {m_msColor, m_msDepth};
    glDeleteFramebuffers(5, fbos);
    glDeleteTextures(5, texs);
    glDeleteRenderbuffers(2, rbs);
    m_msFbo = m_resolveFbo = m_shaftFbo = 0;
}

void PostProcess::resize(int w, int h) {
    if (w == m_w && h == m_h) return;
    if (w <= 0 || h <= 0) return;
    if (m_msFbo) destroy();
    m_w = w;
    m_h = h;
    m_hw = w / 2 > 0 ? w / 2 : 1;
    m_hh = h / 2 > 0 ? h / 2 : 1;

    GLint maxSamples = 4;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    GLsizei samples = maxSamples < 4 ? maxSamples : 4;

    // 1. MSAA HDR target
    glGenRenderbuffers(1, &m_msColor);
    glBindRenderbuffer(GL_RENDERBUFFER, m_msColor);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA16F, w, h);
    glGenRenderbuffers(1, &m_msDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, m_msDepth);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, w, h);
    glGenFramebuffers(1, &m_msFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_msFbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_msColor);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_msDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("MSAA framebuffer incomplete");

    // 2. resolve target: colour + depth textures (depth is read by the light shafts)
    m_hdrTex = makeTexture(w, h, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR);
    m_depthTex = makeTexture(w, h, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT, GL_NEAREST);
    glGenFramebuffers(1, &m_resolveFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_resolveFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_hdrTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Resolve framebuffer incomplete");

    // 3. half resolution targets
    for (int i = 0; i < 2; ++i) {
        m_halfTex[i] = makeTexture(m_hw, m_hh, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR);
        m_halfFbo[i] = makeColorFbo(m_halfTex[i]);
    }
    m_shaftTex = makeTexture(m_hw, m_hh, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR);
    m_shaftFbo = makeColorFbo(m_shaftTex);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcess::beginScene() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_msFbo);
    glViewport(0, 0, m_w, m_h);
}

void PostProcess::endScene() const {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolveFbo);
    glBlitFramebuffer(0, 0, m_w, m_h, 0, 0, m_w, m_h, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcess::drawFullscreen() const {
    glBindVertexArray(m_emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

// Ping-pong Gaussian blur at half resolution. Result ends up in srcTex's FBO
// (srcTex must be m_halfTex[0] or m_shaftTex), m_halfTex[1] is scratch.
void PostProcess::blur(GLuint srcTex, int iterations) {
    GLuint srcFbo = (srcTex == m_shaftTex) ? m_shaftFbo : m_halfFbo[0];
    m_blur->use();
    m_blur->set("uImage", 0);
    glActiveTexture(GL_TEXTURE0);
    for (int i = 0; i < iterations; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_halfFbo[1]);
        glBindTexture(GL_TEXTURE_2D, srcTex);
        m_blur->set("uDirection", glm::vec2(1.0f / m_hw, 0.0f));
        drawFullscreen();
        glBindFramebuffer(GL_FRAMEBUFFER, srcFbo);
        glBindTexture(GL_TEXTURE_2D, m_halfTex[1]);
        m_blur->set("uDirection", glm::vec2(0.0f, 1.0f / m_hh));
        drawFullscreen();
    }
}

void PostProcess::apply(const PostSettings& s, int fbW, int fbH) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glViewport(0, 0, m_hw, m_hh);

    if (s.bloom) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_halfFbo[0]);
        m_bright->use();
        m_bright->set("uHdr", 0);
        m_bright->set("uThreshold", 1.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_hdrTex);
        drawFullscreen();
        blur(m_halfTex[0], 4);
    }

    if (s.shafts) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_shaftFbo);
        m_shafts->use();
        m_shafts->set("uDepth", 0);
        m_shafts->set("uShadowMap", 1);
        m_shafts->set("uInvViewProj", s.invViewProj);
        m_shafts->set("uLightSpace", s.lightSpace);
        m_shafts->set("uCameraPos", s.cameraPos);
        m_shafts->set("uLightDir", s.lightDir);
        m_shafts->set("uBoxMin", s.boxMin);
        m_shafts->set("uBoxMax", s.boxMax);
        m_shafts->set("uResolution", glm::vec2(m_hw, m_hh));
        m_shafts->set("uTime", s.time);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_depthTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, s.shadowMap);
        drawFullscreen();
        blur(m_shaftTex, 1);
    }

    // final composite into the window
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbW, fbH);
    m_composite->use();
    m_composite->set("uHdr", 0);
    m_composite->set("uBloom", 1);
    m_composite->set("uShafts", 2);
    m_composite->set("uBloomEnabled", s.bloom);
    m_composite->set("uShaftsEnabled", s.shafts);
    m_composite->set("uBloomStrength", 0.55f);
    m_composite->set("uShaftColor", s.shaftColor);
    m_composite->set("uExposure", s.exposure);
    m_composite->set("uGrade", s.grade);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hdrTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_halfTex[0]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_shaftTex);
    drawFullscreen();
    glActiveTexture(GL_TEXTURE0);
}
