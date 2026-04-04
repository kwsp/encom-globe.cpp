#version 440

layout(location = 0) in vec2 vUV;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;           // offset 0
    vec3 position;      // offset 64
    float time;         // offset 76
    float size;         // offset 80
    float cameraDistance; // offset 84
    float _pad1;        // offset 88
    float _pad2;        // offset 92
    vec3 waveColor;     // offset 96
    float arcAngle;     // offset 108
} ubuf;

#define PI 3.14159265359

void main()
{
    vec2 uv = vUV * 2.0 - 1.0;
    float dist = length(uv);
    
    // Discard outside circle
    if (dist > 1.0) {
        discard;
    }
    
    // Calculate angle for arc effects
    float angle = atan(uv.y, uv.x);
    
    // Time in seconds (input is milliseconds)
    float t = ubuf.time / 1000.0;
    
    // Initialize output
    vec3 color = vec3(0.0);
    float alpha = 0.0;
    
    // === Core: red circle ring (not filled) ===
    float coreOuterRadius = 0.08;
    float coreInnerRadius = 0.04;
    float coreRing = smoothstep(coreOuterRadius + 0.01, coreOuterRadius, dist) 
                   * smoothstep(coreInnerRadius - 0.01, coreInnerRadius, dist);
    color += vec3(1.0, 0.2, 0.2) * coreRing;
    alpha = max(alpha, coreRing);
    
    // === Rotating shield arcs (thicker white arcs) ===
    float shieldRadius = 0.2;
    float shieldThickness = 0.035;
    float shieldAngle = t * 2.5;  // Rotate over time
    float arcHalfWidth = PI / 4.5;  // 40 degree arcs
    
    // 4 arcs rotating
    for (int i = 0; i < 4; i++) {
        float arcAngle = shieldAngle + float(i) * PI / 2.0;
        float angleDiff = abs(mod(angle - arcAngle + PI, 2.0 * PI) - PI);
        float arcMask = 1.0 - smoothstep(arcHalfWidth * 0.7, arcHalfWidth, angleDiff);
        float ring = smoothstep(shieldRadius + shieldThickness, shieldRadius, dist) 
                   * smoothstep(shieldRadius - shieldThickness, shieldRadius, dist);
        float shield = ring * arcMask * 0.95;
        color += vec3(1.0) * shield;
        alpha = max(alpha, shield);
    }
    
    // === Outward pulsing waves (white arcs pointing toward globe) ===
    int numWaves = 3;
    float waveDuration = 1.5;  // seconds for full expansion
    float waveInterval = 0.5;  // seconds between waves
    float waveHalfAngle = PI / 8.0;  // 22.5 degree half-angle = 45 degree total arc
    
    for (int i = 0; i < numWaves; i++) {
        // Each wave starts at a different time offset
        float wavePhase = mod(t / waveDuration - float(i) * waveInterval / waveDuration, 1.0);
        
        // Wave radius grows from core to edge
        float waveRadius = mix(0.15, 0.95, wavePhase);
        
        // Fade out as wave expands
        float waveAlpha = (1.0 - wavePhase) * 0.85;
        
        // Wave ring thickness (thicker)
        float waveThickness = 0.02 + 0.01 * wavePhase;
        float waveRing = smoothstep(waveRadius + waveThickness, waveRadius, dist)
                        * smoothstep(waveRadius - waveThickness, waveRadius, dist);
        
        // Arc points toward globe using arcAngle from uniform
        float angleDiff = abs(mod(angle - ubuf.arcAngle + PI, 2.0 * PI) - PI);
        float arcMask = 1.0 - smoothstep(waveHalfAngle * 0.6, waveHalfAngle, angleDiff);
        
        float wave = waveRing * arcMask * waveAlpha;
        // White waves
        color += vec3(1.0, 1.0, 1.0) * wave;
        alpha = max(alpha, wave);
    }
    
    // Fog calculation
    float depth = gl_FragCoord.z / gl_FragCoord.w;
    float fogNear = ubuf.cameraDistance;
    float fogFar = ubuf.cameraDistance + 300.0;
    float fogFactor = smoothstep(fogNear, fogFar, depth);
    
    // Fade the alpha for fog
    fragColor = vec4(color, alpha * (1.0 - fogFactor));
}
