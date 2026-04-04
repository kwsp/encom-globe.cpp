#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    float cameraDistance;
} ubuf;

layout(location = 0) out vec3 vColor;

void main() {
    gl_Position = ubuf.mvp * vec4(position, 1.0);
    vColor = color;
}
