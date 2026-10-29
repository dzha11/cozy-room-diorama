#include "Texture.h"
#include "Shader.h"  // resourcePath

#include <stb_image.h>
#include <cstring>
#include <stdexcept>

// Not part of the GL 3.3 core headers but supported on macOS, Windows and Linux
// drivers through GL_EXT_texture_filter_anisotropic.
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

static bool hasAnisotropy() {
    static int cached = -1;
    if (cached < 0) {
        cached = 0;
        GLint n = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &n);
        for (GLint i = 0; i < n; ++i) {
            const char* e = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
            if (e && (std::strcmp(e, "GL_EXT_texture_filter_anisotropic") == 0 ||
                      std::strcmp(e, "GL_ARB_texture_filter_anisotropic") == 0)) {
                cached = 1;
                break;
            }
        }
    }
    return cached == 1;
}

Texture::Texture(const std::string& relativePath, bool srgb) {
    stbi_set_flip_vertically_on_load(true);  // OpenGL expects v = 0 at the bottom
    int channels = 0;
    unsigned char* data = stbi_load(resourcePath(relativePath).c_str(), &m_w, &m_h, &channels, 4);
    if (!data) throw std::runtime_error("Cannot load texture: " + relativePath);

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, m_w, m_h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (hasAnisotropy()) {
        GLfloat maxAniso = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso < 8.0f ? maxAniso : 8.0f);
    }
    stbi_image_free(data);
}

Texture::~Texture() {
    if (m_id) glDeleteTextures(1, &m_id);
}

void Texture::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}
