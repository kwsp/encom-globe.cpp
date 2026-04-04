#version 440

layout(location = 0) in vec2 vUV;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec3 position;
    float time;
    float size;
    vec3 waveColor;
    float padding;
} ubuf;

void main()
{
    // Simple test: solid red circle
    vec2 uv = vUV * 2.0 - 1.0;
    float dist = length(uv);
    
    if (dist > 1.0) {
        discard;
    }
    
    // Red core
    if (dist < 0.3) {
        fragColor = vec4(1.0, 0.0, 0.0, 1.0);
    } else {
        // White ring
        fragColor = vec4(1.0, 1.0, 1.0, 0.8);
    }
}
