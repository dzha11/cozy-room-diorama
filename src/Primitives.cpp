#include "Primitives.h"

#include <glm/gtc/constants.hpp>
#include <cmath>

namespace prim {
namespace {

// Makes every triangle counter-clockwise when seen from the side its vertex
// normals point to, so back-face culling works for all generated shapes.
void fixWinding(const std::vector<Vertex>& v, std::vector<uint32_t>& idx) {
    for (size_t i = 0; i + 2 < idx.size(); i += 3) {
        const glm::vec3& a = v[idx[i]].position;
        const glm::vec3& b = v[idx[i + 1]].position;
        const glm::vec3& c = v[idx[i + 2]].position;
        glm::vec3 faceN = glm::cross(b - a, c - a);
        glm::vec3 avgN = v[idx[i]].normal + v[idx[i + 1]].normal + v[idx[i + 2]].normal;
        if (glm::dot(faceN, avgN) < 0.0f) std::swap(idx[i + 1], idx[i + 2]);
    }
}

std::unique_ptr<Mesh> finish(std::vector<Vertex>& v, std::vector<uint32_t>& idx) {
    fixWinding(v, idx);
    return std::make_unique<Mesh>(v, idx);
}

}  // namespace

std::unique_ptr<Mesh> box(const glm::vec3& size, float uvPerMeter) {
    struct Face { glm::vec3 n, u, v; };
    const Face faces[6] = {
        {{ 1, 0, 0}, { 0, 0, -1}, {0, 1, 0}},
        {{-1, 0, 0}, { 0, 0,  1}, {0, 1, 0}},
        {{ 0, 1, 0}, { 1, 0,  0}, {0, 0, -1}},
        {{ 0,-1, 0}, { 1, 0,  0}, {0, 0, 1}},
        {{ 0, 0, 1}, { 1, 0,  0}, {0, 1, 0}},
        {{ 0, 0,-1}, {-1, 0,  0}, {0, 1, 0}},
    };
    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;
    const glm::vec3 half = size * 0.5f;
    for (const Face& f : faces) {
        float du = std::abs(glm::dot(f.u, size));
        float dv = std::abs(glm::dot(f.v, size));
        glm::vec3 centre = f.n * std::abs(glm::dot(f.n, half));
        uint32_t base = static_cast<uint32_t>(verts.size());
        const float st[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        for (auto& c : st) {
            glm::vec3 p = centre + f.u * ((c[0] - 0.5f) * du) + f.v * ((c[1] - 0.5f) * dv);
            glm::vec2 uv = uvPerMeter > 0.0f ? glm::vec2(c[0] * du, c[1] * dv) * uvPerMeter
                                             : glm::vec2(c[0], c[1]);
            verts.push_back({p, f.n, uv});
        }
        idx.insert(idx.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
    }
    return finish(verts, idx);
}

std::unique_ptr<Mesh> plane(float w, float d, float rx, float rz) {
    std::vector<Vertex> v = {
        {{-w / 2, 0, d / 2}, {0, 1, 0}, {0, 0}},
        {{ w / 2, 0, d / 2}, {0, 1, 0}, {rx, 0}},
        {{ w / 2, 0, -d / 2}, {0, 1, 0}, {rx, rz}},
        {{-w / 2, 0, -d / 2}, {0, 1, 0}, {0, rz}},
    };
    std::vector<uint32_t> idx = {0, 1, 2, 0, 2, 3};
    return finish(v, idx);
}

std::unique_ptr<Mesh> quad(float w, float h) {
    std::vector<Vertex> v = {
        {{-w / 2, -h / 2, 0}, {0, 0, 1}, {0, 0}},
        {{ w / 2, -h / 2, 0}, {0, 0, 1}, {1, 0}},
        {{ w / 2,  h / 2, 0}, {0, 0, 1}, {1, 1}},
        {{-w / 2,  h / 2, 0}, {0, 0, 1}, {0, 1}},
    };
    std::vector<uint32_t> idx = {0, 1, 2, 0, 2, 3};
    return finish(v, idx);
}

std::unique_ptr<Mesh> sphere(float r, int seg, int rings) {
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    const float PI = glm::pi<float>();
    for (int y = 0; y <= rings; ++y) {
        float phi = PI * y / rings;  // 0 at the north pole
        for (int x = 0; x <= seg; ++x) {
            float theta = 2.0f * PI * x / seg;
            glm::vec3 n(std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta));
            v.push_back({n * r, n, {float(x) / seg, 1.0f - float(y) / rings}});
        }
    }
    for (int y = 0; y < rings; ++y)
        for (int x = 0; x < seg; ++x) {
            uint32_t a = y * (seg + 1) + x, b = a + seg + 1;
            idx.insert(idx.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    return finish(v, idx);
}

std::unique_ptr<Mesh> cylinder(float rb, float rt, float h, int seg, bool caps) {
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    const float PI = glm::pi<float>();
    const float slope = (rb - rt) / h;  // makes cone normals tilt correctly
    for (int i = 0; i <= seg; ++i) {
        float a = 2.0f * PI * i / seg;
        float c = std::cos(a), s = std::sin(a);
        glm::vec3 n = glm::normalize(glm::vec3(c, slope, s));
        v.push_back({{rb * c, 0, rb * s}, n, {float(i) / seg, 0}});
        v.push_back({{rt * c, h, rt * s}, n, {float(i) / seg, 1}});
    }
    for (int i = 0; i < seg; ++i) {
        uint32_t b0 = 2 * i, t0 = b0 + 1, b1 = b0 + 2, t1 = b0 + 3;
        idx.insert(idx.end(), {b0, t0, t1, b0, t1, b1});
    }
    if (caps) {
        for (int cap = 0; cap < 2; ++cap) {
            float y = cap ? h : 0.0f, r = cap ? rt : rb;
            glm::vec3 n(0, cap ? 1.0f : -1.0f, 0);
            uint32_t centre = static_cast<uint32_t>(v.size());
            v.push_back({{0, y, 0}, n, {0.5f, 0.5f}});
            for (int i = 0; i <= seg; ++i) {
                float a = 2.0f * PI * i / seg;
                v.push_back({{r * std::cos(a), y, r * std::sin(a)}, n,
                             {0.5f + 0.5f * std::cos(a), 0.5f + 0.5f * std::sin(a)}});
            }
            for (int i = 0; i < seg; ++i) idx.insert(idx.end(), {centre, centre + 1 + i, centre + 2 + i});
        }
    }
    return finish(v, idx);
}

std::unique_ptr<Mesh> torus(float R, float r, int seg, int sides) {
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    const float PI = glm::pi<float>();
    for (int i = 0; i <= seg; ++i) {
        float u = 2.0f * PI * i / seg;
        for (int j = 0; j <= sides; ++j) {
            float w = 2.0f * PI * j / sides;
            glm::vec3 n(std::cos(w) * std::cos(u), std::cos(w) * std::sin(u), std::sin(w));
            glm::vec3 p((R + r * std::cos(w)) * std::cos(u), (R + r * std::cos(w)) * std::sin(u), r * std::sin(w));
            v.push_back({p, n, {float(i) / seg, float(j) / sides}});
        }
    }
    for (int i = 0; i < seg; ++i)
        for (int j = 0; j < sides; ++j) {
            uint32_t a = i * (sides + 1) + j, b = a + sides + 1;
            idx.insert(idx.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    return finish(v, idx);
}

}  // namespace prim
