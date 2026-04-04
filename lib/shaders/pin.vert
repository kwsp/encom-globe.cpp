#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec3 viewDir;
} ubuf;

layout(location = 0) out vec3 vColor;
layout(location = 1) out float vRelativeDepth;

void main() {
    vRelativeDepth = dot(normalize(position), ubuf.viewDir);
    gl_Position = ubuf.mvp * vec4(position, 1.0);
    vColor = color;
}
