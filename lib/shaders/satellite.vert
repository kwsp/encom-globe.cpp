#version 440

layout(location = 0) in vec2 offset;     // Billboard corner offset (-0.5 to 0.5)

layout(std140, binding = 0) uniform buf {
    mat4 mvp;           // offset 0
    vec3 position;      // offset 64
    float time;         // offset 76
    float size;         // offset 80
    float _pad1[3];     // offset 84  (align viewDir to 96)
    vec3 viewDir;       // offset 96
    float _pad2;        // offset 108 (align waveColor to 112)
    vec3 waveColor;     // offset 112
    float arcAngle;     // offset 124
    vec3 coreColor;     // offset 128
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
