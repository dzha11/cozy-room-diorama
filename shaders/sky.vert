#version 330 core
// Backdrop quad behind the window. The sky colour is computed per pixel in
// sky.frag from the viewing direction, so it shows parallax like a real sky.
layout(location = 0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vWorldPos;

void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    gl_Position = uProj * uView * world;
}
