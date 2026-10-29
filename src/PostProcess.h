#pragma once
// PostProcess: owns the off-screen framebuffers.
//   1. multisampled HDR target (RGBA16F + depth, 4x MSAA) for the scene
//   2. resolved HDR colour + depth textures
//   3. half-resolution ping-pong targets for bloom and light shafts
// and runs bloom, light shafts and the final tone-mapping composite.

#include "Shader.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <memory>

struct PostSettings {
    bool bloom = true;
    bool shafts = true;
    float exposure = 1.0f;
    glm::vec3 grade{1.0f};
    glm::vec3 shaftColor{0.0f};
    // light shafts input
    glm::mat4 invViewProj{1.0f};
    glm::mat4 lightSpace{1.0f};
    glm::vec3 cameraPos{0.0f};
    glm::vec3 lightDir{0, 1, 0};
    glm::vec3 boxMin{0.0f}, boxMax{0.0f};
    GLuint shadowMap = 0;
    float time = 0.0f;
};

class PostProcess {
public:
    PostProcess();
    ~PostProcess();
    PostProcess(const PostProcess&) = delete;
    PostProcess& operator=(const PostProcess&) = delete;

    void resize(int width, int height);
    void beginScene() const;   // bind MSAA HDR target
    void endScene() const;     // resolve MSAA into textures
    void apply(const PostSettings& s, int fbWidth, int fbHeight);   // draws to the default framebuffer

private:
    void destroy();
    void blur(GLuint srcTex, int iterations);
    void drawFullscreen() const;

    int m_w = 0, m_h = 0, m_hw = 0, m_hh = 0;
    GLuint m_msFbo = 0, m_msColor = 0, m_msDepth = 0;
    GLuint m_resolveFbo = 0, m_hdrTex = 0, m_depthTex = 0;
    GLuint m_halfFbo[2] = {0, 0}, m_halfTex[2] = {0, 0};
    GLuint m_shaftFbo = 0, m_shaftTex = 0;
    GLuint m_emptyVao = 0;

    std::unique_ptr<Shader> m_bright, m_blur, m_shafts, m_composite;
};
