#version 330 core
// Depth-only pass from the sun's point of view (shadow map).
layout(location = 0) in vec3 aPos;
layout(location = 3) in mat4 aInstanceModel;

uniform mat4 uModel;
uniform mat4 uLightSpace;
uniform bool uInstanced;

void main() {
    mat4 model = uInstanced ? aInstanceModel : uModel;
    gl_Position = uLightSpace * model * vec4(aPos, 1.0);
}
