# Encom Globe - QtQuick + Qt ShaderTools Port

## Overview
A C++ port of the Encom globe visualization from Tron: Legacy, originally implemented in Three.js/WebGL. This version uses **QtQuick + Qt ShaderTools** for pure QML/C++ rendering without Qt Widgets or Qt3D.

## Architecture

### Rendering Approach: QSGRenderNode
Uses Qt Quick Scene Graph with custom RHI (Rendering Hardware Interface) nodes for direct GPU access. This provides:
- Cross-platform shaders via QSB (OpenGL, Vulkan, Metal, D3D)
- Efficient rendering without FBO copies
- Native QtQuick integration

### Component Hierarchy
```
QQuickItem (Globe)
├── QSGRenderNode (GlobeRenderer)
│   ├── Hex tile mesh with intro animation shader
│   └── Smoke particle system
├── QSGRenderNode (SatelliteRenderer)
│   └── Texture atlas animated satellites
├── QQuickItem (Pin)
│   └── Line geometry + label sprite
└── QQuickItem (Marker)
    └── Arc line geometry + marker sprite
```

## Build

```bash
cd /Users/tnie/code/agent/globe.cpp
cmake --preset clang -B build-clang
cmake --build build-clang --config Debug
./build-clang/src/Debug/EncomGlobe
```

## Project Structure

```
src/
├── main.cpp              # Qt application entry
├── Main.qml              # Root QML with Globe item
├── Globe.h/cpp           # Globe QQuickItem
├── GlobeRenderer.h/cpp   # QSGRenderNode for globe mesh
├── Satellite.h/cpp       # Satellite QQuickItem + renderer
├── Pin.h/cpp             # Location pin markers
├── Marker.h/cpp          # Connected flight path markers
├── SmokeSystem.h/cpp     # GPU particle system
├── Utils.h/cpp           # Coordinate conversion, etc.
├── shaders/
│   ├── globe.vert        # Hex tile vertex shader (intro animation)
│   ├── globe.frag        # Hex tile fragment shader (fog)
│   ├── satellite.vert    # Satellite billboard shader
│   ├── satellite.frag    # Satellite texture animation
│   ├── smoke.vert        # Particle vertex shader
│   └── smoke.frag        # Particle fragment shader
└── CMakeLists.txt
```

## Shader Porting: Three.js → Qt RHI

### Key Differences

| Aspect | Three.js GLSL | Qt RHI GLSL |
|--------|---------------|-------------|
| Version | `#version 300 es` | `#version 440` |
| Attributes | `attribute vec3 position` | `layout(location=0) in vec3 position` |
| Uniforms | `uniform float time` | `layout(std140, binding=0) uniform buf { float time; }` |
| Varyings | `varying vec4 vColor` | `layout(location=0) out vec4 vColor` |
| Matrix | Separate MVP | Pre-computed in uniform buffer |

### Globe Vertex Shader (Qt RHI)

```glsl
#version 440
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in float lng;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    float currentTime;
    float introDuration;
    float introAltitude;
    float cameraDistance;
} ubuf;

layout(location = 0) out vec4 vColor;

void main() {
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
        newPos = position * (1.0 + ((ubuf.introAltitude - 1.0) * (1.0 - t)));
    }
    
    vColor = vec4(color, opacityVal);
    gl_Position = ubuf.mvp * vec4(newPos, 1.0);
}
```

## Features Implemented

### Core Globe
- [x] Hexagonal tile generation (icosphere subdivision)
- [x] Color variation per tile
- [x] Camera rotation animation
- [ ] Intro animation shader (longitude-based reveal)
- [ ] Fog effect

### Visual Elements
- [ ] Satellites with texture atlas animation
- [ ] Pin markers with labels and smoke
- [ ] Connected markers with arc lines
- [ ] Intro lines animation

## Qt Components Used

| Class | Purpose |
|-------|---------|
| `QQuickItem` | Custom QML item base class |
| `QSGRenderNode` | Direct RHI rendering in scene graph |
| `QRhi` | Qt Rendering Hardware Interface |
| `QRhiBuffer` | GPU vertex/uniform buffers |
| `QRhiGraphicsPipeline` | Shader pipeline state |
| `QRhiTexture` | Texture resources |
| `QShader` | Compiled QSB shaders |

## Coordinate System

Matches original Three.js:
- **Radius**: 500 units
- **Camera distance**: 1700 units
- **Lat/Lon → XYZ**: Standard spherical to Cartesian conversion

```cpp
QVector3D latLonToXYZ(float lat, float lon, float scale = 500.0f) {
    float phi = qDegreesToRadians(90.0f - lat);
    float theta = qDegreesToRadians(180.0f - lon);
    return {
        scale * std::sin(phi) * std::cos(theta),
        scale * std::cos(phi),
        scale * std::sin(phi) * std::sin(theta)
    };
}
```

## Implementation Roadmap

### Phase 1: Core Globe ✓
1. CMake configuration with ShaderTools
2. Globe QQuickItem + QSGRenderNode
3. Icosphere hex tile generation
4. Globe shaders (intro animation + fog)
5. Camera rotation

### Phase 2: Visual Elements
6. Satellite with texture atlas
7. Pin with line + label
8. Marker with arc lines

### Phase 3: Effects
9. Smoke particle system
10. Intro lines
11. QML API for customization

## Original vs Port

| Aspect | Original (Three.js) | Port (Qt RHI) |
|--------|--------------------| --------------|
| Geometry | BufferGeometry | QRhiBuffer |
| Shaders | Custom GLSL | QSB (cross-platform) |
| Animation | requestAnimationFrame | QSGRenderNode::render() |
| UI | HTML/CSS | QtQuick/QML |
| Intro reveal | Longitude-based shader | Same approach |
| Satellites | Canvas sprite sheet | QRhiTexture atlas |

## References

- Original: `~/code/agent/encom-globe/`
- Port: `~/code/agent/globe.cpp/`
- Qt RHI: https://doc.qt.io/qt-6/qrhi.html
- Qt ShaderTools: https://doc.qt.io/qt-6/qtshadertools-index.html
