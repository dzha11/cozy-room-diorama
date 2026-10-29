#version 330 core
// Volumetric light shafts: march along the camera ray inside the room and
// count how much of it is lit by the sun (shadow map lookups). The result is
// the amount of in-scattered light, shaped by a Henyey-Greenstein phase
// function (more light when looking towards the sun).
// Phase function: Henyey & Greenstein (1941). Noise: J. Jimenez, "Next
// Generation Post Processing in Call of Duty: Advanced Warfare", SIGGRAPH 2014.

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uDepth;
uniform sampler2DShadow uShadowMap;
uniform mat4 uInvViewProj;
uniform mat4 uLightSpace;
uniform vec3 uCameraPos;
uniform vec3 uLightDir;      // towards the sun
uniform vec3 uBoxMin;
uniform vec3 uBoxMax;
uniform vec2 uResolution;
uniform float uTime;

const int STEPS = 40;

// interleaved gradient noise (Jimenez 2014): hides banding between steps
float ign(vec2 p) { return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715)))); }

bool rayBox(vec3 ro, vec3 rd, out float t0, out float t1) {
    vec3 inv = 1.0 / rd;
    vec3 a = (uBoxMin - ro) * inv, b = (uBoxMax - ro) * inv;
    vec3 lo = min(a, b), hi = max(a, b);
    t0 = max(max(lo.x, lo.y), max(lo.z, 0.0));
    t1 = min(min(hi.x, hi.y), hi.z);
    return t1 > t0;
}

void main() {
    float depth = texture(uDepth, vUV).r;
    vec4 ndc = vec4(vUV * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 wp = uInvViewProj * ndc;
    vec3 world = wp.xyz / wp.w;

    vec3 rd = world - uCameraPos;
    float sceneDist = length(rd);
    rd /= sceneDist;

    float t0, t1;
    if (!rayBox(uCameraPos, rd, t0, t1)) { FragColor = vec4(0.0); return; }
    t1 = min(t1, sceneDist);
    if (t1 <= t0) { FragColor = vec4(0.0); return; }

    float stepLen = (t1 - t0) / float(STEPS);
    float t = t0 + stepLen * ign(gl_FragCoord.xy + fract(uTime) * 60.0);
    float lit = 0.0;
    for (int i = 0; i < STEPS; ++i) {
        vec3 p = uCameraPos + rd * t;
        vec4 ls = uLightSpace * vec4(p, 1.0);
        vec3 s = ls.xyz / ls.w * 0.5 + 0.5;
        lit += texture(uShadowMap, vec3(s.xy, s.z - 0.001));
        t += stepLen;
    }
    float scattered = lit * stepLen;   // lit distance in metres

    float g = 0.55;
    float cosT = dot(rd, normalize(uLightDir));
    float phase = (1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * cosT, 1.5) / (4.0 * 3.14159265);
    FragColor = vec4(vec3(scattered * phase), 1.0);
}
