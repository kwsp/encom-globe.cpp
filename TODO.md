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

## Phase 2: Visual Elements 🔄
- [x] SatelliteRenderer QSGRenderNode with procedural shader
- [x] Integration into Globe rendering tree
- [ ] Test satellite animation and positioning
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

## Known Issues
- SatelliteRenderer not fully tested yet - need to verify positioning and animation
- Need to implement proper billboard rotation to face camera

## Architecture Notes
- GlobeRenderer and SatelliteRenderer are both children of the root QSGNode
- Each renderer manages its own pipeline, buffers, and shader resources
- Globe class coordinates the renderers via updatePaintNode
- All shaders use std140 layout for cross-platform uniform buffer compatibility
