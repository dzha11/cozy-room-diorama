#version 330 core
// Bloom step 1: keep only the bright parts of the HDR image (soft threshold).
// Bloom pipeline (bright pass + ping-pong blur) after LearnOpenGL "Bloom".
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uHdr;
uniform float uThreshold;

void main() {
    vec3 c = texture(uHdr, vUV).rgb;
    float lum = max(c.r, max(c.g, c.b));
    float knee = uThreshold * 0.5;
    float soft = clamp(lum - uThreshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 1e-4);
    float w = max(soft, lum - uThreshold) / max(lum, 1e-4);
    FragColor = vec4(c * w, 1.0);
}
