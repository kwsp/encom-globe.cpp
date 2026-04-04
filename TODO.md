# Encom Globe Port - TODO

## Phase 1: Core Globe ✅
- [x] CMakeLists.txt with ShaderTools
- [x] Shader files (globe.vert, globe.frag)
- [x] Utils (latLonToXYZ, color utilities, elasticOut)
- [x] Globe QQuickItem
- [x] GlobeRenderer QSGRenderNode
- [x] Hexagonal tile mesh from grid.json
- [x] Camera rotation animation
- [x] Intro animation shader

## Phase 2: Visual Elements ✅
- [x] SatelliteRenderer with procedural shader (pulsing waves, shield arcs)
- [x] Multiple satellites with dynamic uniform buffer offsets
- [x] PinRenderer with line geometry + billboard tops
- [x] Elastic bounce animation for pins
- [x] Projected 2D text labels via QML Repeater
- [x] Back-face culling for labels (dot product visibility)

## Phase 3: Markers & Effects ✅
- [x] Marker with arc line drawing (connected nodes)
- [x] Smoke particle system (GPU-animated particles)
- [x] Intro lines animation (sweeping lines synced with globe)
- [x] Color customization API (Q_PROPERTY)

## Phase 4: Polish
- [ ] Performance optimization (instancing)
- [ ] Memory management
- [ ] Cross-platform testing
- [ ] Documentation

## Color API
All colors configurable from QML:
- `baseColor` - Globe tile base color (default: "#ffcc00")
- `pinColor` - Pin line and top color (default: "#00eeee")
- `markerColor` - Marker sprite and arc line color (default: "#ffcc00")
- `satelliteColor` - Satellite core color (default: "#ff0000")
- `introLinesColor` - Intro lines color (default: "#8FD8D8")

## Architecture
- 6 QSGRenderNode children: Globe, Satellite, Pin, Marker, Smoke, IntroLines
- Globe coordinates renderers via updatePaintNode (render thread)
- Globe::updateState() handles logic on main thread (QTimer 16ms)
- Pin/Marker labels projected to 2D via MVP, exposed as QVariantList property
- QML Repeater renders native Text items for crisp labels
- Dynamic uniform buffer offsets for multi-instance drawing
- All shaders use std140 layout for cross-platform compatibility
- Camera starts at angle PI to match original encom-globe
