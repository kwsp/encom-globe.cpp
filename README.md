# Encom Globe in C++/Qt Quick

A high-performance C++ port of [encom-globe](https://github.com/arscan/encom-globe) using **Qt 6**, **Qt Quick**, and **Qt RHI**.

This project demonstrates how to build complex 3D visualizations in Qt, leveraging Qt's modern Rendering Hardware Interface (RHI) for cross-platform compatibility (Metal on macOS, Vulkan on Linux, D3D on Windows).

## Features

- **Modern RHI Architecture**: Uses `QSGRenderNode` for seamless integration with the Qt Quick Scene Graph.
- **Visual Fidelity**:
    - **Hexagonal Tile Globe**: Optimized mesh generation from JSON with longitude-based sweep entry.
    - **Satellites**: Procedural billboard shaders with pulsing waves and rotating shields.
    - **Pins & Markers**: Animated 3D lines and arc connections with elastic bounce effects.
    - **Smoke System**: GPU-animated particle system rising from active nodes.
    - **Intro Animation**: Synchronized swirling lines and globe materialization.
- **Hybrid Rendering**: 3D elements rendered via RHI, while labels use native Qt Quick `Text` items for pixel-perfect clarity.
- **Interactive**: Full mouse support for rotation (drag), tilt (drag), and zoom (scroll).
- ️ **Configurable**: Exposes a rich QML API for colors, sizes, and animation durations.

## Technical Highlights

### 1. RHI-Based Shaders
All shaders are written in **Vulkan-style GLSL (440)** and compiled into `.qsb` (Qt Shader Binary) files using the `qt_add_shaders` CMake macro. This allows the same shader code to run on Metal, Vulkan, and OpenGL.

### 2. std140 Layout & Alignment
The project strictly follows `std140` uniform buffer layout rules to ensure memory alignment compatibility across all graphics APIs.
- Handled `vec3` alignment issues by adding explicit padding.
- Used `uniformBufferWithDynamicOffset` for efficient multi-instance rendering (satellites/markers).

### 3. Dynamic Uniform Buffers
Instead of updating the same uniform buffer multiple times per frame (which causes synchronization bottlenecks), the project uses **Dynamic Offsets**:
1. Allocate one large buffer for all instances.
2. Upload all data in a single batch.
3. Draw each instance using a specific memory offset.

### 4. Projected 2D Labels
To avoid blurry 3D text, labels are calculated by projecting 3D world coordinates back to 2D screen space using the MVP matrix. These coordinates are exposed to QML, allowing a standard `Repeater` to manage high-quality native Text items.

## Building and Running

### Prerequisites
- Qt 6.5+ (Qt 6.11 recommended)
- CMake 3.16+
- A compiler supporting C++20

### Build Steps
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Running
```bash
# macOS
./src/EncomGlobe.app/Contents/MacOS/EncomGlobe

# Linux/Windows
./src/EncomGlobe
```

## QML API

| Property | Type | Description |
|----------|------|-------------|
| `baseColor` | `string` | The primary color of the globe tiles. |
| `pinColor` | `string` | Color of Pins and their heads. |
| `markerColor` | `string` | Color of Markers and Arc Lines. |
| `satelliteColor`| `string` | Core color of orbiting satellites. |
| `introDuration` | `real` | Duration of the intro animation in ms. |
| `markerSize` | `real` | Scale factor for marker sprites. |
| `rotationOffset`| `real` | Manual rotation adjustment (for dragging). |

| Method | Description |
|--------|-------------|
| `addPin(lat, lon, text)` | Adds a vertical pin at the location. |
| `addMarker(lat, lon, text, connected)` | Adds a node, optionally connected to the last one. |
| `addSatellite(lat, lon, alt)` | Launches a satellite at a specific altitude. |

## License
MIT License - Ported with AI assistance.
Original JavaScript implementation by [Rob Scanlon](https://github.com/arscan).
