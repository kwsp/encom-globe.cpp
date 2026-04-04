#version 440

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec3 vColor;
layout(location = 2) in float vRelativeDepth;

layout(location = 0) out vec4 fragColor;

void main() {
    vec2 uv = vUV * 2.0 - 1.0;
    float dist = length(uv);
    if (dist > 1.0) {
        discard;
    }
    
    float alpha = smoothstep(1.0, 0.85, dist);
    
    // Normalized Fog
    float fogFactor = smoothstep(0.0, -0.6, vRelativeDepth);
    
    fragColor = vec4(vColor, alpha * (1.0 - fogFactor));
}
