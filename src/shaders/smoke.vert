#version 440

layout(location = 0) in float startLat;
layout(location = 1) in float startLon;
layout(location = 2) in float altitude;
layout(location = 3) in float startTime;
layout(location = 4) in float isActive;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec4 color;
    float currentTime;
    float cameraDistance;
} ubuf;

layout(location = 0) out vec4 vColor;

#define PI 3.14159265358979
#define RADIUS 500.0

vec3 getPos(float lat, float lon, float alt) {
    float phi = (90.0 - lat) * PI / 180.0;
    float theta = (180.0 - lon) * PI / 180.0;
    return vec3(
        RADIUS * sin(phi) * cos(theta) * alt,
        RADIUS * cos(phi) * alt,
        RADIUS * sin(phi) * sin(theta) * alt
    );
}

void main() {
    float dt = ubuf.currentTime - startTime;
    if (dt < 0.0) dt = 0.0;
    
    // Cycle every 1.5 seconds
    if (isActive > 0.5) {
        dt = mod(dt, 1500.0);
    } else {
        dt = 0.0;
    }
    
    float opacity = 1.0 - (dt / 1500.0);
    if (isActive < 0.5 || dt == 0.0) opacity = 0.0;
    
    // Slight drift in longitude and rising altitude
    float currentAlt = altitude + (dt / 1500.0) * 0.1;
    float currentLon = startLon - (dt / 50.0);
    
    vec3 pos = getPos(startLat, currentLon, currentAlt);
    
    gl_Position = ubuf.mvp * vec4(pos, 1.0);
    gl_PointSize = 6.0 * opacity;
    vColor = vec4(ubuf.color.rgb, opacity * 0.5);
}
