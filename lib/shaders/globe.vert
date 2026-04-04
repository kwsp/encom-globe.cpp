#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in float lng;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;           // offset 0
    vec3 viewDir;       // offset 64
    float currentTime;  // offset 76
    float introDuration;// offset 80
    float introAltitude;// offset 84
    vec2 padding;       // offset 88
} ubuf;

layout(location = 0) out vec4 vColor;
layout(location = 1) out float vRelativeDepth;

#define GLOBE_RADIUS 500.0

void main()
{
    vec3 newPos = position;
    float opacityVal = 0.0;
    float introStart = ubuf.introDuration * ((180.0 + lng) / 360.0);

    if (ubuf.currentTime > introStart) {
        opacityVal = 1.0;
    }

    if (ubuf.currentTime > introStart && 
        ubuf.currentTime < introStart + ubuf.introDuration / 8.0) {
        newPos = position * ubuf.introAltitude;
        opacityVal = 0.3;
    }

    if (ubuf.currentTime > introStart + ubuf.introDuration / 8.0 && 
        ubuf.currentTime < introStart + ubuf.introDuration / 8.0 + 200.0) {
        float t = (ubuf.currentTime - introStart - ubuf.introDuration / 8.0) / 200.0;
        float scale = 1.0 + ((ubuf.introAltitude - 1.0) * (1.0 - t));
        newPos = position * scale;
    }

    vColor = vec4(color, opacityVal);
    
    // Compute relative depth: 1.0 at front, 0.0 at center, -1.0 at back
    // Use the normalized view direction (pointing from camera to center)
    // Actually, ubuf.viewDir is normalize(cameraPos).
    // So the direction from origin to camera is ubuf.viewDir.
    vRelativeDepth = dot(normalize(position), ubuf.viewDir);
    
    gl_Position = ubuf.mvp * vec4(newPos, 1.0);
}
