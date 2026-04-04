#version 440

layout(location = 0) in vec2 offset;     // Billboard corner offset (-0.5 to 0.5)

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec3 position;
    float time;
    float size;
    vec3 waveColor;
    float padding;
} ubuf;

layout(location = 0) out vec2 vUV;

void main()
{
    // The MVP matrix includes the billboard transform and position
    // Vertex position is just the offset (z=0, w=1)
    gl_Position = ubuf.mvp * vec4(offset, 0.0, 1.0);
    
    // UV coordinates for the billboard quad
    vUV = vec2(offset.x + 0.5, 0.5 - offset.y);
}
