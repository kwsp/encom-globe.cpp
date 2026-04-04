#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in float longitude;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    float opacity;
    float rotation;
    float currentTime;
    float duration;
    float cameraDistance;
} ubuf;

layout(location = 0) out vec4 vColor;

void main() {
    float s = sin(ubuf.rotation);
    float c = cos(ubuf.rotation);
    mat4 rotY = mat4(
        c, 0.0, -s, 0.0,
        0.0, 1.0, 0.0, 0.0,
        s, 0.0, c, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
    
    gl_Position = ubuf.mvp * rotY * vec4(position, 1.0);
    vColor = vec4(color, ubuf.opacity * 0.8);
}
