#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in float longitude;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;           // offset 0
    vec3 viewDir;       // offset 64
    float opacity;      // offset 76
    float rotation;     // offset 80
    float currentTime;  // offset 84
    float duration;     // offset 88
    float padding;      // offset 92
} ubuf;

layout(location = 0) out vec4 vColor;
layout(location = 1) out float vRelativeDepth;

void main() {
    float s = sin(ubuf.rotation);
    float c = cos(ubuf.rotation);
    mat4 rotY = mat4(
        c, 0.0, -s, 0.0,
        0.0, 1.0, 0.0, 0.0,
        s, 0.0, c, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
    
    vec3 rotatedPos = (rotY * vec4(position, 1.0)).xyz;
    vRelativeDepth = dot(normalize(rotatedPos), ubuf.viewDir);
    
    gl_Position = ubuf.mvp * vec4(rotatedPos, 1.0);
    vColor = vec4(color, ubuf.opacity * 0.8);
}
