#version 440

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec3 vColor;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec3 color;
    float cameraDistance;
} ubuf;

void main() {
    vec2 uv = vUV * 2.0 - 1.0;
    float dist = length(uv);
    if (dist > 1.0) {
        discard;
    }
    
    float alpha = smoothstep(1.0, 0.85, dist);
    
    float depth = gl_FragCoord.z / gl_FragCoord.w;
    float fogNear = ubuf.cameraDistance;
    float fogFar = ubuf.cameraDistance + 300.0;
    float fogFactor = smoothstep(fogNear, fogFar, depth);
    
    // Fade alpha for fog
    fragColor = vec4(vColor, alpha * (1.0 - fogFactor));
}
