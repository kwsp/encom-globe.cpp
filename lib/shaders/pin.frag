#version 440

layout(location = 0) in vec3 vColor;
layout(location = 1) in float vRelativeDepth;
layout(location = 0) out vec4 fragColor;

void main() {
    // Normalized Fog
    float fogFactor = smoothstep(0.0, -0.6, vRelativeDepth);
    fragColor = vec4(vColor, 1.0 - fogFactor);
}
