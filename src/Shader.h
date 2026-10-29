#pragma once
// Shader: loads a GLSL vertex + fragment pair from shaders/*.vert / *.frag,
// compiles and links it, and offers cached uniform setters.

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

class Shader {
public:
    Shader() = default;
    Shader(const std::string& vertFile, const std::string& fragFile);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const { glUseProgram(m_id); }
    GLuint id() const { return m_id; }

    void set(const std::string& name, int v) const;
    void set(const std::string& name, bool v) const { set(name, v ? 1 : 0); }
    void set(const std::string& name, float v) const;
    void set(const std::string& name, const glm::vec2& v) const;
    void set(const std::string& name, const glm::vec3& v) const;
    void set(const std::string& name, const glm::vec4& v) const;
    void set(const std::string& name, const glm::mat3& m) const;
    void set(const std::string& name, const glm::mat4& m) const;

private:
    GLint location(const std::string& name) const;
    static GLuint compile(GLenum type, const std::string& src, const std::string& file);

    GLuint m_id = 0;
    mutable std::unordered_map<std::string, GLint> m_cache;
};

// Absolute path of a file inside the project (shaders/, assets/).
std::string resourcePath(const std::string& relative);
