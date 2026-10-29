#version 330 core
// Main scene vertex shader: MVP transform, world-space normal, UVs and the
// position in light space for shadow mapping. Supports instanced drawing.

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in mat4 aInstanceModel;   // locations 3..6
layout(location = 7) in vec4 aInstanceColor;

uniform mat4 uModel;
uniform mat3 uNormalMat;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uLightSpace;
uniform bool uInstanced;
uniform float uWorldUV;   // > 0: planar UVs from world position

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out vec4 vLightSpacePos;
out vec4 vInstanceColor;

void main() {
    mat4 model = uInstanced ? aInstanceModel : uModel;
    mat3 normalMat = uInstanced ? transpose(inverse(mat3(model))) : uNormalMat;

    vec4 world = model * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    vNormal = normalize(normalMat * aNormal);
    vInstanceColor = uInstanced ? aInstanceColor : vec4(1.0, 1.0, 1.0, 0.0);

    if (uWorldUV > 0.0) {
        // project the world position on the plane the face lies in, so
        // neighbouring wall pieces continue the same pattern without seams
        vec3 n = abs(vNormal);
        vec2 p = (n.y > 0.5) ? world.xz : (n.x > 0.5 ? world.zy : world.xy);
        vUV = p * uWorldUV;
    } else {
        vUV = aUV;
    }

    vLightSpacePos = uLightSpace * world;
    gl_Position = uProj * uView * world;
}
