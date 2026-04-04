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

### Running the sample
```bash
# macOS
./src/EncomGlobe.app/Contents/MacOS/EncomGlobe

# Linux/Windows
./src/EncomGlobe
```

## Integration in C++/CMake

To use `EncomGlobeLib` in your own project, follow these steps:

### 1. CMake Setup
Add the library and its plugin to your `target_link_libraries`. You must link both the backing library and the generated QML plugin:

```cmake
target_link_libraries(MyProject PRIVATE
    EncomGlobeLib
    EncomGlobeLibplugin
)
```

### 2. C++ Registration
Because the library is static, you must manually import the QML plugin in your `main.cpp` using the `Q_IMPORT_QML_PLUGIN` macro. This ensures the linker doesn't discard the registration code.

```cpp
#include <QtQml/qqmlextensionplugin.h>

// Import the static EncomGlobe QML plugin
Q_IMPORT_QML_PLUGIN(EncomGlobePlugin)

int main(int argc, char *argv[]) {
    // ...
}
```

### 3. Usage in QML

To use the globe in your application, import the `EncomGlobe` module and add the `Globe` element. You can then interact with it using its properties and methods.

```qml
import EncomGlobe

Globe {
    id: myGlobe
    anchors.fill: parent
    
    baseColor: "#ffcc00"
    showLabels: true
    
    Component.onCompleted: {
        addPin(40.7128, -74.0060, "New York")
        addMarker(34.0522, -118.2437, "Los Angeles", true)
    }
}
```

For a complete example including a full control panel and interaction setup, please refer to [example/Main.qml](example/Main.qml).

## QML API

### Properties

| Property | Type | Unit / Range | Description |
|----------|------|--------------|-------------|
| `dayLength` | `real` | ms | Rotation speed (ms per full 360° turn). |
| `scale` | `real` | multiplier | Zoom factor (typical range: 0.3 - 3.0). |
| `viewAngle` | `real` | radians | Vertical camera tilt/angle. |
| `rotationOffset`| `real` | radians | Manual horizontal rotation adjustment. |
| `baseColor` | `string` | hex string | The primary color of the globe tiles. |
| `pinColor` | `string` | hex string | Color of Pins and their heads. |
| `markerColor` | `string` | hex string | Color of Markers and Arc Lines. |
| `satelliteColor`| `string` | hex string | Core color of orbiting satellites. |
| `introLinesColor`| `string` | hex string | Color of the initial sweeping intro lines. |
| `introDuration` | `real` | ms | Duration of the intro animation. |
| `markerSize` | `real` | multiplier | Scale factor for marker sprites. |
| `pinHeadSize` | `real` | multiplier | Scale factor for the circular heads of pins. |
| `showLabels` | `bool` | flag | Whether to display the 2D text labels. |

### Methods

| Method | Description |
|--------|-------------|
| `addPin(lat, lon, text)` | Adds a vertical pin at the specified lat/lon (degrees). |
| `addMarker(lat, lon, text, connected)` | Adds a node, optionally connected to the last one by an arc. |
| `addSatellite(lat, lon, alt)` | Launches a satellite at a specific altitude multiplier (e.g. 1.2). |
| `clearSatellites()` | Removes all active satellites. |

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

## License
MIT License - Ported with AI assistance.
Original JavaScript implementation by [Rob Scanlon](https://github.com/arscan).
