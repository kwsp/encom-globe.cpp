#version 440

layout(location = 0) in vec2 offset;     // Billboard corner offset (-0.5 to 0.5)

layout(std140, binding = 0) uniform buf {
    mat4 mvp;           // offset 0
    vec3 position;      // offset 64
    float time;         // offset 76
    float size;         // offset 80
    vec3 viewDir;       // offset 84
    vec3 waveColor;     // offset 96
    float arcAngle;     // offset 108
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
