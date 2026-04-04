#version 440

layout(location = 0) in vec2 offset;     // Billboard corner offset (-0.5 to 0.5)

layout(std140, binding = 0) uniform buf {
    mat4 mvp;           // offset 0,   64 bytes
    vec3 position;      // offset 64,  12 bytes
    float time;         // offset 76,  4 bytes
    float size;         // offset 80,  4 bytes
    float _p1;          // offset 84,  4 bytes
    float _p2;          // offset 88,  4 bytes
    float _p3;          // offset 92,  4 bytes
    vec3 viewDir;       // offset 96,  12 bytes
    float _p4;          // offset 108, 4 bytes
    vec3 waveColor;     // offset 112, 12 bytes
    float arcAngle;     // offset 124, 4 bytes
    vec3 coreColor;     // offset 128, 12 bytes
} ubuf;

layout(location = 0) out vec2 vUV;
layout(location = 1) out float vRelativeDepth;

void main()
{
    // Relative depth for fog
    vRelativeDepth = dot(normalize(ubuf.position), ubuf.viewDir);
    
    // The MVP matrix includes the billboard transform and position
    gl_Position = ubuf.mvp * vec4(offset, 0.0, 1.0);
    
    // UV coordinates for the billboard quad
    vUV = vec2(offset.x + 0.5, 0.5 - offset.y);
}
