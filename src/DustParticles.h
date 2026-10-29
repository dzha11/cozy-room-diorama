#pragma once
// Dust motes floating in the room. All particles are drawn with ONE instanced
// draw call; positions are animated on the GPU (see shaders/dust.vert).

#include "Shader.h"
#include <glad/gl.h>
#include <glm/glm.hpp>

class DustParticles {
public:
    explicit DustParticles(int count = 1800);
    ~DustParticles();
    DustParticles(const DustParticles&) = delete;
    DustParticles& operator=(const DustParticles&) = delete;

    struct DrawParams {
        glm::mat4 view, proj, lightSpace;
        glm::vec3 boxMin, boxMax;
        glm::vec3 lightColor, ambient;
        float time, amount;
        GLuint shadowMap;
        bool shadowsEnabled;
    };
    void draw(const DrawParams& p) const;
    int count() const { return m_count; }

private:
    int m_count;
    GLuint m_vao = 0, m_quadVbo = 0, m_seedVbo = 0;
    Shader m_shader;
};
