#include "DustParticles.h"

#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <vector>

DustParticles::DustParticles(int count) : m_count(count), m_shader("dust.vert", "dust.frag") {
    const float corners[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};  // two triangles

    std::mt19937 rng(1234);
    std::uniform_real_distribution<float> U(0.0f, 1.0f);
    std::vector<glm::vec4> seeds(count);
    for (auto& s : seeds) s = glm::vec4(U(rng), U(rng), U(rng), U(rng));

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_quadVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    // per-instance seed, uploaded once
    glGenBuffers(1, &m_seedVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_seedVbo);
    glBufferData(GL_ARRAY_BUFFER, seeds.size() * sizeof(glm::vec4), seeds.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);
    glVertexAttribDivisor(1, 1);

    glBindVertexArray(0);
}

DustParticles::~DustParticles() {
    glDeleteBuffers(1, &m_quadVbo);
    glDeleteBuffers(1, &m_seedVbo);
    glDeleteVertexArrays(1, &m_vao);
}

void DustParticles::draw(const DrawParams& p) const {
    // camera right/up vectors from the view matrix rows -> billboards
    glm::vec3 right(p.view[0][0], p.view[1][0], p.view[2][0]);
    glm::vec3 up(p.view[0][1], p.view[1][1], p.view[2][1]);

    m_shader.use();
    m_shader.set("uView", p.view);
    m_shader.set("uProj", p.proj);
    m_shader.set("uLightSpace", p.lightSpace);
    m_shader.set("uBoxMin", p.boxMin);
    m_shader.set("uBoxMax", p.boxMax);
    m_shader.set("uCamRight", right);
    m_shader.set("uCamUp", up);
    m_shader.set("uTime", p.time);
    m_shader.set("uSize", 0.010f);
    m_shader.set("uLightColor", p.lightColor);
    m_shader.set("uAmbient", p.ambient);
    m_shader.set("uAmount", p.amount);
    m_shader.set("uShadowsEnabled", p.shadowsEnabled);
    m_shader.set("uShadowMap", 5);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, p.shadowMap);
    glActiveTexture(GL_TEXTURE0);

    // additive blending, depth test on (hidden behind furniture), no depth write
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);
    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, m_count);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
