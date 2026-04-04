#version 440

layout(location = 0) in vec2 offset;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec3 color;
    float relativeDepth;
    vec3 viewDir;
} ubuf;

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec3 vColor;
layout(location = 2) out float vRelativeDepth;

void main() {
    vRelativeDepth = ubuf.relativeDepth;
    gl_Position = ubuf.mvp * vec4(offset, 0.0, 1.0);
    vUV = vec2(offset.x + 0.5, 0.5 - offset.y);
    vColor = ubuf.color;
}
