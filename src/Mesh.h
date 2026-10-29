#pragma once
// Mesh: owns a VAO + VBO + EBO. Optionally owns an instance buffer
// (per-instance model matrix + colour) for instanced drawing.

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct InstanceData {
    glm::mat4 model;
    glm::vec4 color;   // rgb tint, a = extra per-instance value (e.g. twinkle phase)
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void draw() const;

    // Uploads the instance buffer once (static data, GL_STATIC_DRAW).
    void setInstances(const std::vector<InstanceData>& instances);
    void drawInstanced() const;
    int instanceCount() const { return m_instanceCount; }

    GLuint vao() const { return m_vao; }

private:
    GLuint m_vao = 0, m_vbo = 0, m_ebo = 0, m_instanceVbo = 0;
    GLsizei m_indexCount = 0;
    int m_instanceCount = 0;
};
