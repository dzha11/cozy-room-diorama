#version 330 core
// Final pass: HDR scene + bloom + light shafts -> exposure -> ACES filmic
// tone mapping -> time-of-day colour grade -> vignette -> gamma.
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uHdr;
uniform sampler2D uBloom;
uniform sampler2D uShafts;
uniform bool uBloomEnabled;
uniform bool uShaftsEnabled;
uniform float uBloomStrength;
uniform vec3 uShaftColor;
uniform float uExposure;
uniform vec3 uGrade;

// ACES filmic curve fit by K. Narkowicz, "ACES Filmic Tone Mapping Curve" (2015)
vec3 aces(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 c = texture(uHdr, vUV).rgb;
    if (uBloomEnabled) c += texture(uBloom, vUV).rgb * uBloomStrength;
    if (uShaftsEnabled) c += texture(uShafts, vUV).r * uShaftColor;

    c *= uExposure;
    c *= uGrade;
    c = aces(c);

    vec2 q = vUV - 0.5;
    c *= mix(1.0, 0.72, smoothstep(0.25, 0.75, dot(q, q) * 2.0));   // vignette

    c = pow(c, vec3(1.0 / 2.2));
    FragColor = vec4(c, 1.0);
}
