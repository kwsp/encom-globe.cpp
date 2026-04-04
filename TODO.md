# Encom Globe Port - TODO

## Phase 1: Core Globe ✅
- [x] Update CMakeLists.txt with ShaderTools
- [x] Create shader files (globe.vert, globe.frag)
- [x] Implement Utils (latLonToXYZ, color utilities)
- [x] Implement Globe QQuickItem
- [x] Implement GlobeRenderer QSGRenderNode
- [x] Generate hexagonal tile mesh from grid.json
- [x] Camera rotation animation
- [x] Test intro animation shader

## Phase 2: Visual Elements ✅
- [x] SatelliteRenderer QSGRenderNode with procedural shader
- [x] Integration into Globe rendering tree
- [x] Multiple satellites visible at different positions
- [x] Animated pulsing wave effects on satellites
- [ ] Pin QQuickItem with line geometry
- [ ] Marker with arc line drawing
- [ ] Label rendering (SDF or sprite)

## Phase 3: Effects
- [ ] Smoke particle system
- [ ] Intro lines animation
- [ ] Color customization API (Q_PROPERTY)
- [ ] Mouse interaction (drag to rotate)

## Phase 4: Polish
- [ ] Performance optimization (instancing for satellites)
- [ ] Memory management
- [ ] Cross-platform testing (macOS Metal, Linux Vulkan/OpenGL, Windows D3D11)
- [ ] Documentation and examples

## Key Technical Insights

### Dynamic Uniform Buffers in QRhi
When drawing multiple objects with different uniforms:
1. Use `uniformBufferWithDynamicOffset` binding (requires size parameter)
2. Create buffer with space for all objects (256-byte aligned chunks typical)
3. Upload ALL uniform data in ONE `QRhiResourceUpdateBatch` before drawing
4. Use `QRhiCommandBuffer::DynamicOffset{binding, offset}` for each draw
5. Don't interleave `updateDynamicBuffer` with `draw` calls - batch first!

### Qt 6.11 API Changes
- `QRhiCommandBuffer::DynamicOffset` is a struct `{int binding, quint32 offset}`
- `uniformBufferWithDynamicOffset(binding, stages, buffer, size)` requires size

## Architecture Notes
- GlobeRenderer and SatelliteRenderer are both children of the root QSGNode
- Each renderer manages its own pipeline, buffers, and shader resources
- Globe class coordinates the renderers via updatePaintNode
- All shaders use std140 layout for cross-platform uniform buffer compatibility
- Satellites use billboard rendering with camera-facing orientation
