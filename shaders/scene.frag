#version 330 core
// Main scene fragment shader: per-fragment Blinn-Phong lighting with
//   - one directional light (sun or moon) with PCF shadow mapping
//   - up to MAX_POINT_LIGHTS point lights (floor lamp, fairy lights, laptop)
//   - one spotlight (desk lamp) with a soft cone edge
//   - hemispheric ambient light (sky colour from above, floor bounce below)
// Output is linear HDR colour; tone mapping happens in composite.frag.
//
// References: Blinn-Phong and PCF shadow mapping follow the structure of the
// LearnOpenGL tutorials (J. de Vries, learnopengl.com, CC BY-NC 4.0), rewritten
// for several light types; the light falloff window is from B. Karis,
// "Real Shading in Unreal Engine 4", SIGGRAPH 2013 course notes.

#define MAX_POINT_LIGHTS 24

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec4 vLightSpacePos;
in vec4 vInstanceColor;

out vec4 FragColor;

struct PointLight { vec3 position; vec3 color; float radius; };
struct SpotLight { vec3 position; vec3 direction; vec3 color; float radius; float innerCos; float outerCos; };

// material
uniform vec3 uAlbedo;
uniform bool uHasTexture;
uniform sampler2D uTexture;
uniform float uSpecular;
uniform float uShininess;
uniform vec3 uEmissive;
uniform bool uTextureIsEmissive;
uniform float uAlpha;
uniform bool uInstanced;
uniform float uTime;

// lights
uniform vec3 uCameraPos;
uniform vec3 uDirLightDir;      // towards the light
uniform vec3 uDirLightColor;
uniform vec3 uAmbientSky;
uniform vec3 uAmbientGround;
uniform int uNumPointLights;
uniform PointLight uPointLights[MAX_POINT_LIGHTS];
uniform SpotLight uSpot;

// shadows
uniform bool uShadowsEnabled;
uniform sampler2DShadow uShadowMap;
uniform vec2 uShadowTexel;

// Smooth window so a light reaches exactly zero at its radius
// (inverse-square falloff multiplied by a windowing function).
float attenuation(float dist, float radius) {
    float x = clamp(1.0 - pow(dist / radius, 4.0), 0.0, 1.0);
    return (x * x) / (dist * dist + 1.0);
}

vec3 blinnPhong(vec3 N, vec3 V, vec3 L, vec3 lightColor, vec3 albedo) {
    float diff = max(dot(N, L), 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), uShininess) * uSpecular;
    spec *= step(0.0, dot(N, L));   // no highlight on the back side
    return lightColor * (albedo * diff + vec3(spec));
}

// 3x3 PCF using hardware-filtered depth comparisons (each tap is bilinear),
// with a normal offset to avoid shadow acne.
float shadowFactor(vec3 N, vec3 L) {
    if (!uShadowsEnabled) return 1.0;
    vec3 p = vLightSpacePos.xyz / vLightSpacePos.w;
    p = p * 0.5 + 0.5;
    if (p.z > 1.0 || p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0) return 1.0;
    float cosT = clamp(dot(N, L), 0.0, 1.0);
    float bias = mix(0.0012, 0.0002, cosT);
    float sum = 0.0;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            sum += texture(uShadowMap, vec3(p.xy + vec2(x, y) * uShadowTexel * 1.25, p.z - bias));
    return sum / 9.0;
}

void main() {
    vec4 texel = uHasTexture ? texture(uTexture, vUV) : vec4(1.0);
    vec3 albedo = uAlbedo * texel.rgb * vInstanceColor.rgb;

    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing) N = -N;          // open shapes (lamp shades) are lit on both sides
    vec3 V = normalize(uCameraPos - vWorldPos);

    // hemispheric ambient
    float up = N.y * 0.5 + 0.5;
    vec3 color = albedo * mix(uAmbientGround, uAmbientSky, up);

    // sun / moon
    vec3 L = normalize(uDirLightDir);
    color += blinnPhong(N, V, L, uDirLightColor, albedo) * shadowFactor(N, L);

    // point lights
    for (int i = 0; i < uNumPointLights; ++i) {
        vec3 toL = uPointLights[i].position - vWorldPos;
        float d = length(toL);
        if (d > uPointLights[i].radius) continue;
        color += blinnPhong(N, V, toL / d, uPointLights[i].color, albedo) * attenuation(d, uPointLights[i].radius);
    }

    // desk lamp spotlight
    {
        vec3 toL = uSpot.position - vWorldPos;
        float d = length(toL);
        vec3 Ls = toL / d;
        float theta = dot(-Ls, normalize(uSpot.direction));
        float cone = smoothstep(uSpot.outerCos, uSpot.innerCos, theta);
        if (cone > 0.0 && d < uSpot.radius)
            color += blinnPhong(N, V, Ls, uSpot.color, albedo) * cone * attenuation(d, uSpot.radius);
    }

    // emission (bulbs, lamp shades, laptop screen)
    vec3 emissive = uEmissive;
    if (uInstanced) {
        float twinkle = 0.8 + 0.2 * sin(uTime * 2.3 + vInstanceColor.a);
        emissive *= vInstanceColor.rgb * twinkle;
    }
    if (uTextureIsEmissive) emissive *= texel.rgb;
    color += emissive;

    FragColor = vec4(color, uAlpha);
}
