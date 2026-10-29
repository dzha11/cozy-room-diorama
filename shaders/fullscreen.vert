#version 330 core
// Full-screen triangle generated from gl_VertexID (no vertex buffer needed,
// an empty VAO is bound). Used by all post-processing passes.
out vec2 vUV;

void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vUV = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
