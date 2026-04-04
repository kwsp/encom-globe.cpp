#version 440

layout(location = 0) in vec2 vUV;
layout(location = 1) in float vRelativeDepth;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;           // offset 0,   64 bytes
    vec3 position;      // offset 64,  12 bytes
    float time;         // offset 76,  4 bytes
    float size;         // offset 80,  4 bytes
    float _p1;          // offset 84,  4 bytes
    float _p2;          // offset 88,  4 bytes
    float _p3;          // offset 92,  4 bytes
    vec3 viewDir;       // offset 96,  12 bytes
    float _p4;          // offset 108, 4 bytes
    vec3 waveColor;     // offset 112, 12 bytes
    float arcAngle;     // offset 124, 4 bytes
    vec3 coreColor;     // offset 128, 12 bytes
} ubuf;

#define PI 3.14159265359

void main()
{
    vec2 uv = vUV * 2.0 - 1.0;
    float dist = length(uv);
    
    if (dist > 1.0) {
        discard;
    }
    
    float angle = atan(uv.y, uv.x);
    
    // Original encom logic constants
    float numFrames = 50.0;
    float waveStart = 6.0;
    float numWaves = 8.0;
    float repeatAt = 40.0;
    float swirlDone = 23.0;
    float arcBuffer = PI / 16.0;
    float start = -PI + PI / 4.0;
    
    // Map time to 0..50 frame range (matched to 80ms per frame from original)
    float timeInFrames = ubuf.time / 80.0;
    float frame = timeInFrames;
    if (frame >= repeatAt) {
        frame = repeatAt + mod(frame - repeatAt, numFrames - repeatAt);
    }
    
    vec3 color = vec3(0.0);
    float alpha = 0.0;
    
    // === Core: red circle ring ===
    float coreOuterRadius = 0.08;
    float coreInnerRadius = 0.04;
    float coreRing = smoothstep(coreOuterRadius + 0.01, coreOuterRadius, dist) 
                   * smoothstep(coreInnerRadius - 0.01, coreInnerRadius, dist);
    color += ubuf.coreColor * coreRing;
    alpha = max(alpha, coreRing);
    
    // === Shield Arcs (Exact Encom Logic) ===
    float shieldRadius = 0.2;
    float shieldThickness = 0.06;
    
    for (int n = 0; n < 4; n++) {
        float arcStartAngle = 0.0;
        float arcEndAngle = 0.0;
        float baseOffset = float(n) * PI / 2.0 + start;
        
        if (frame < waveStart || frame >= numFrames) {
            arcStartAngle = baseOffset + arcBuffer;
            arcEndAngle = baseOffset + PI / 2.0 - arcBuffer;
        } else if (frame < swirlDone) {
            float total = swirlDone - waveStart;
            float distToGo = 3.0 * PI / 2.0;
            float step = frame - waveStart;
            float movement = distToGo / total;
            float movingStart = start + arcBuffer + movement * step;
            arcStartAngle = max(baseOffset, movingStart);
            arcEndAngle = max(baseOffset + PI / 2.0 - 2.0 * arcBuffer, movingStart + PI / 2.0 - 2.0 * arcBuffer);
        } else if (frame < repeatAt) {
            float total = repeatAt - swirlDone;
            float distToGo = float(n) * 2.0 * PI / 4.0;
            float step = frame - swirlDone;
            float movement = distToGo / total;
            float movingStart = PI / 2.0 + PI / 4.0 + arcBuffer + movement * step;
            arcStartAngle = movingStart;
            arcEndAngle = movingStart + PI / 2.0 - 2.0 * arcBuffer;
        } else if (frame < (numFrames - repeatAt) / 2.0 + repeatAt) {
            float total = (numFrames - repeatAt) / 2.0;
            float distToGo = PI / 2.0;
            float step = frame - repeatAt;
            float movement = distToGo / total;
            float movingStart = float(n) * (PI / 2.0) + PI / 4.0 + arcBuffer + movement * step;
            arcStartAngle = movingStart;
            arcEndAngle = movingStart + PI / 2.0 - 2.0 * arcBuffer;
        } else {
            arcStartAngle = baseOffset + arcBuffer;
            arcEndAngle = baseOffset + PI / 2.0 - arcBuffer;
        }
        
        float diff = mod(angle - arcStartAngle, 2.0 * PI);
        float arcLen = mod(arcEndAngle - arcStartAngle, 2.0 * PI);
        
        if (diff < arcLen) {
            float ring = smoothstep(shieldRadius + shieldThickness, shieldRadius, dist) 
                       * smoothstep(shieldRadius - shieldThickness, shieldRadius, dist);
            color += vec3(1.0) * ring * 0.95;
            alpha = max(alpha, ring);
        }
    }
    
    // === Outward pulsing waves ===
    int numWavesCount = 3;
    float t = ubuf.time / 1000.0;
    float waveDuration = 1.5;
    float waveInterval = 0.5;
    float waveHalfAngle = PI / 8.0;
    
    for (int i = 0; i < numWavesCount; i++) {
        float wavePhase = mod(t / waveDuration - float(i) * waveInterval / waveDuration, 1.0);
        float waveRadius = mix(0.15, 0.95, wavePhase);
        float waveAlpha = 1.2 * (1.0 - wavePhase * 0.5);
        float waveThickness = 0.04 + 0.02 * wavePhase;
        float waveRing = smoothstep(waveRadius + waveThickness, waveRadius, dist)
                        * smoothstep(waveRadius - waveThickness, waveRadius, dist);
        float angleDiff = abs(mod(angle - ubuf.arcAngle + PI, 2.0 * PI) - PI);
        float arcMask = 1.0 - smoothstep(waveHalfAngle * 0.6, waveHalfAngle, angleDiff);
        float wave = waveRing * arcMask * waveAlpha;
        color += vec3(1.0, 1.0, 1.0) * wave;
        alpha = max(alpha, wave);
    }
    
    // Normalized Fog calculation
    float fogFactor = smoothstep(0.0, -0.6, vRelativeDepth);
    fragColor = vec4(color, alpha * (1.0 - fogFactor));
}
