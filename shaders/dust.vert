#version 330 core
// Dust particles: one instanced camera-facing quad per particle.
// Motion is computed here from a per-particle seed and time, so the particle
// buffer is uploaded once and never changes (no per-frame uploads).
// Each particle samples the shadow map: it is only bright inside the sun beam.

layout(location = 0) in vec2 aCorner;     // quad corner -1..1
layout(location = 1) in vec4 aSeed;       // xyz = position 0..1 in the room box, w = random

uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uLightSpace;
uniform vec3 uBoxMin;
uniform vec3 uBoxMax;
uniform vec3 uCamRight;
uniform vec3 uCamUp;
uniform float uTime;
uniform float uSize;
uniform sampler2DShadow uShadowMap;
uniform bool uShadowsEnabled;

out vec2 vCorner;
out float vLit;
out float vAlpha;

void main() {
    float r = aSeed.w;
    // slow drift + gentle swirl, wrapped inside the room box
    vec3 p = aSeed.xyz + vec3(0.010, -0.004 - 0.004 * r, 0.006) * uTime;
    p += 0.015 * vec3(sin(uTime * (0.3 + r) + r * 40.0), sin(uTime * 0.21 + r * 17.0), cos(uTime * (0.25 + r * 0.5) + r * 9.0));
    p = fract(p);
    vec3 world = mix(uBoxMin, uBoxMax, p);

    // light visibility from the shadow map (1 = in the beam)
    vec4 ls = uLightSpace * vec4(world, 1.0);
    vec3 s = ls.xyz / ls.w * 0.5 + 0.5;
    float lit = 1.0;
    if (uShadowsEnabled && s.x > 0.0 && s.x < 1.0 && s.y > 0.0 && s.y < 1.0)
        lit = texture(uShadowMap, vec3(s.xy, s.z - 0.002));
    vLit = lit;

    // twinkle as the flakes rotate
    vAlpha = 0.55 + 0.45 * sin(uTime * (1.0 + 2.0 * r) + r * 60.0);

    float size = uSize * (0.6 + 0.8 * r);
    vec3 pos = world + (uCamRight * aCorner.x + uCamUp * aCorner.y) * size;
    vCorner = aCorner;
    gl_Position = uProj * uView * vec4(pos, 1.0);
}
