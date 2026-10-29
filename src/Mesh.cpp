#include "Mesh.h"

#include <cstddef>

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : m_indexCount(static_cast<GLsizei>(indices.size())) {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // layout(location = 0) position, 1 normal, 2 uv
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

    glBindVertexArray(0);
}

Mesh::~Mesh() {
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
    if (m_instanceVbo) glDeleteBuffers(1, &m_instanceVbo);
    glDeleteVertexArrays(1, &m_vao);
}

void Mesh::draw() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
}

void Mesh::setInstances(const std::vector<InstanceData>& instances) {
    m_instanceCount = static_cast<int>(instances.size());
    glBindVertexArray(m_vao);
    if (!m_instanceVbo) glGenBuffers(1, &m_instanceVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(InstanceData), instances.data(), GL_STATIC_DRAW);

    // layout(location = 3..6) mat4 model (one vec4 per column), 7 colour
    for (int c = 0; c < 4; ++c) {
        glEnableVertexAttribArray(3 + c);
        glVertexAttribPointer(3 + c, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                              (void*)(offsetof(InstanceData, model) + sizeof(glm::vec4) * c));
        glVertexAttribDivisor(3 + c, 1);
    }
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, color));
    glVertexAttribDivisor(7, 1);
    glBindVertexArray(0);
}

void Mesh::drawInstanced() const {
    if (m_instanceCount == 0) return;
    glBindVertexArray(m_vao);
    glDrawElementsInstanced(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr, m_instanceCount);
}
