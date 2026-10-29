#pragma once
// Directional-light shadow map: depth texture rendered from the sun/moon with
// an orthographic projection fitted tightly around the room.

#include <glad/gl.h>
#include <glm/glm.hpp>

class ShadowMap {
public:
    explicit ShadowMap(int size = 2048);
    ~ShadowMap();
    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    // Builds the light view-projection that encloses [boundsMin, boundsMax].
    void fit(const glm::vec3& lightDir, const glm::vec3& boundsMin, const glm::vec3& boundsMax);

    void begin() const;   // binds the FBO, sets viewport, clears depth
    void end() const;

    const glm::mat4& lightSpace() const { return m_lightSpace; }
    GLuint depthTexture() const { return m_depth; }
    int size() const { return m_size; }

private:
    int m_size;
    GLuint m_fbo = 0, m_depth = 0;
    glm::mat4 m_lightSpace{1.0f};
};
