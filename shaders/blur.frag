#version 330 core
// Separable Gaussian blur (9 taps folded into 5 bilinear fetches).
// Run once horizontally and once vertically per iteration.
// Weights/offsets for linear sampling: D. Rakos, "Efficient Gaussian blur with
// linear sampling" (rastergrid.com, 2010).
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uImage;
uniform vec2 uDirection;   // (1/width, 0) or (0, 1/height)

void main() {
    const float w0 = 0.2270270270;
    const float w1 = 0.3162162162, o1 = 1.3846153846;
    const float w2 = 0.0702702703, o2 = 3.2307692308;
    vec3 c = texture(uImage, vUV).rgb * w0;
    c += texture(uImage, vUV + uDirection * o1).rgb * w1;
    c += texture(uImage, vUV - uDirection * o1).rgb * w1;
    c += texture(uImage, vUV + uDirection * o2).rgb * w2;
    c += texture(uImage, vUV - uDirection * o2).rgb * w2;
    FragColor = vec4(c, 1.0);
}
