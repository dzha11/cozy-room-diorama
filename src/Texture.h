#pragma once
// Texture: 2D texture loaded with stb_image, repeat wrap, trilinear
// mipmapping and anisotropic filtering when the driver supports it.

#include <glad/gl.h>
#include <string>

class Texture {
public:
    // srgb = true for colour textures (albedo, posters) so lighting is done in
    // linear space; false for data textures.
    Texture(const std::string& relativePath, bool srgb = true);
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void bind(int unit) const;
    GLuint id() const { return m_id; }
    int width() const { return m_w; }
    int height() const { return m_h; }

private:
    GLuint m_id = 0;
    int m_w = 0, m_h = 0;
};
