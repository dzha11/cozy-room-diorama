#version 330 core
// Soft round dust mote, added on top of the scene (additive blending).
in vec2 vCorner;
in float vLit;
in float vAlpha;

uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform float uAmount;

out vec4 FragColor;

void main() {
    float r2 = dot(vCorner, vCorner);
    if (r2 > 1.0) discard;
    float falloff = exp(-r2 * 3.0);
    vec3 c = (uLightColor * vLit * 0.9 + uAmbient * 0.04) * falloff * vAlpha * uAmount;
    FragColor = vec4(c, 1.0);
}
