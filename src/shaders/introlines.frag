#version 440

layout(location = 0) in vec4 vColor;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    float opacity;
    float rotation;
    float currentTime;
    float duration;
    float cameraDistance;
} ubuf;

void main() {
    float depth = gl_FragCoord.z / gl_FragCoord.w;
    float fogNear = ubuf.cameraDistance;
    float fogFar = ubuf.cameraDistance + 300.0;
    float fogFactor = smoothstep(fogNear, fogFar, depth);
    
    // Fade alpha for fog
    fragColor = vec4(vColor.rgb, vColor.a * (1.0 - fogFactor));
}
