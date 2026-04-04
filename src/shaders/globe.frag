#version 440

layout(location = 0) in vec4 vColor;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    float currentTime;
    float introDuration;
    float introAltitude;
    float cameraDistance;
    float padding;
} ubuf;

void main()
{
    // Fog calculation
    float depth = gl_FragCoord.z / gl_FragCoord.w;
    float fogNear = ubuf.cameraDistance;
    float fogFar = ubuf.cameraDistance + 300.0;
    float fogFactor = smoothstep(fogNear, fogFar, depth);
    
    // Just fade the alpha instead of mixing with black
    fragColor = vec4(vColor.rgb, vColor.a * (1.0 - fogFactor));
}
