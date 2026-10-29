#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#ifndef RESOURCE_DIR
#define RESOURCE_DIR "."
#endif

std::string resourcePath(const std::string& relative) {
    // Prefer files next to the working directory (useful for a packaged copy),
    // otherwise fall back to the source tree path baked in by CMake.
    std::ifstream local(relative);
    if (local.good()) return relative;
    return std::string(RESOURCE_DIR) + "/" + relative;
}

static std::string readFile(const std::string& path) {
    std::ifstream in(resourcePath(path));
    if (!in) throw std::runtime_error("Cannot open shader file: " + path);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

GLuint Shader::compile(GLenum type, const std::string& src, const std::string& file) {
    GLuint s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s, 1, &c, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        glDeleteShader(s);
        throw std::runtime_error("Shader compile error in " + file + ":\n" + log);
    }
    return s;
}

Shader::Shader(const std::string& vertFile, const std::string& fragFile) {
    GLuint vs = compile(GL_VERTEX_SHADER, readFile("shaders/" + vertFile), vertFile);
    GLuint fs = compile(GL_FRAGMENT_SHADER, readFile("shaders/" + fragFile), fragFile);
    m_id = glCreateProgram();
    glAttachShader(m_id, vs);
    glAttachShader(m_id, fs);
    glLinkProgram(m_id);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = 0;
    glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(m_id, sizeof(log), nullptr, log);
        throw std::runtime_error("Shader link error (" + vertFile + " + " + fragFile + "):\n" + log);
    }
}

Shader::~Shader() {
    if (m_id) glDeleteProgram(m_id);
}

GLint Shader::location(const std::string& name) const {
    auto it = m_cache.find(name);
    if (it != m_cache.end()) return it->second;
    GLint loc = glGetUniformLocation(m_id, name.c_str());
    m_cache[name] = loc;
    return loc;
}

void Shader::set(const std::string& n, int v) const { glUniform1i(location(n), v); }
void Shader::set(const std::string& n, float v) const { glUniform1f(location(n), v); }
void Shader::set(const std::string& n, const glm::vec2& v) const { glUniform2fv(location(n), 1, glm::value_ptr(v)); }
void Shader::set(const std::string& n, const glm::vec3& v) const { glUniform3fv(location(n), 1, glm::value_ptr(v)); }
void Shader::set(const std::string& n, const glm::vec4& v) const { glUniform4fv(location(n), 1, glm::value_ptr(v)); }
void Shader::set(const std::string& n, const glm::mat3& m) const {
    glUniformMatrix3fv(location(n), 1, GL_FALSE, glm::value_ptr(m));
}
void Shader::set(const std::string& n, const glm::mat4& m) const {
    glUniformMatrix4fv(location(n), 1, GL_FALSE, glm::value_ptr(m));
}
