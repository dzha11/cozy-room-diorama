#include "ShadowMap.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

ShadowMap::ShadowMap(int size) : m_size(size) {
    glGenTextures(1, &m_depth);
    glBindTexture(GL_TEXTURE_2D, m_depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // outside the map = lit
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    // hardware depth comparison -> sampler2DShadow with bilinear PCF
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Shadow map framebuffer incomplete");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ShadowMap::~ShadowMap() {
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteTextures(1, &m_depth);
}

void ShadowMap::fit(const glm::vec3& lightDir, const glm::vec3& bmin, const glm::vec3& bmax) {
    glm::vec3 centre = (bmin + bmax) * 0.5f;
    glm::vec3 dir = glm::normalize(lightDir);
    glm::vec3 up = std::abs(dir.y) > 0.99f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    glm::mat4 view = glm::lookAt(centre + dir * 20.0f, centre, up);

    // transform the 8 corners into light space and take their bounds
    glm::vec3 lo(1e9f), hi(-1e9f);
    for (int i = 0; i < 8; ++i) {
        glm::vec3 c((i & 1) ? bmax.x : bmin.x, (i & 2) ? bmax.y : bmin.y, (i & 4) ? bmax.z : bmin.z);
        glm::vec3 v = glm::vec3(view * glm::vec4(c, 1.0f));
        lo = glm::min(lo, v);
        hi = glm::max(hi, v);
    }
    const float m = 0.1f;
    glm::mat4 proj = glm::ortho(lo.x - m, hi.x + m, lo.y - m, hi.y + m, -hi.z - 1.0f, -lo.z + 1.0f);
    m_lightSpace = proj * view;
}

void ShadowMap::begin() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_size, m_size);
    glEnable(GL_DEPTH_TEST);   // depth writes only happen with the depth test on
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);   // slope-scaled depth bias against shadow acne
}

void ShadowMap::end() const {
    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
