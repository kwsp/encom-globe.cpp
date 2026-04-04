#version 440

layout(location = 0) in vec4 vColor;
layout(location = 1) in float vRelativeDepth;

layout(location = 0) out vec4 fragColor;

void main() {
    // Normalized Fog calculation
    // Particles should vanish slightly before the globe tiles
    // Globe fades from 0.0 to -0.6
    // Smoke fades from 0.0 to -0.4 for a more aggressive cutoff
    float fogFactor = smoothstep(0.0, -0.4, vRelativeDepth);
    
    fragColor = vec4(vColor.rgb, vColor.a * (1.0 - fogFactor));
}
