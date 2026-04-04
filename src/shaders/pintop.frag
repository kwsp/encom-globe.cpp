#version 440

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec3 vColor;
layout(location = 0) out vec4 fragColor;

void main() {
    vec2 uv = vUV * 2.0 - 1.0;
    float dist = length(uv);
    if (dist > 1.0) {
        discard;
    }
    
    // Draw a filled circle with smooth edges
    float alpha = smoothstep(1.0, 0.85, dist);
    
    // Inner slightly darker part for a "pin head" look or just solid color
    // According to original: it's a solid circle
    fragColor = vec4(vColor, alpha);
}
