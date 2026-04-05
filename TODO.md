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
- [x] **Normalized Depth Fog**: Resolution-independent alpha fading based on relative depth.
- [x] **Improved Smoke Occlusion**: Fixed particle trails being visible behind the globe via synced fog logic.

## Phase 4: Polish ✅
- [x] Performance optimization (Dynamic uniform offsets)
- [x] Globe tile color variations (Pusher.color matching)
- [x] Mouse interaction (Horizontal/Vertical Drag + Scroll Zoom)
- [x] Memory management (RAII resources)
- [x] Cross-platform testing readiness
- [x] Documentation (README.md)

## Phase 5: Future Optimizations & Refactoring
- [x] **Architectural: Common Base Class**: Centralize shared state (`mvp`, `viewDir`, `viewportRect`) into a `GlobeRenderNode` base class to reduce boilerplate.
- [x] **Base Class Helpers**: `loadShader()`, `alphaBlend()`, `copyMvp()`, `billboardMatrix()` — eliminated ~176 lines of duplicated code across 6 renderers.
- [x] **Performance: Frame-rate Independence**: Converted fixed 16ms logic to a VSync-driven variable delta-time (`dt`) system for perfect smoothness on high-refresh displays.
- [x] **Sequential Marker Animation**: Each marker arc waits for its predecessor to finish drawing before starting.
- [x] **Dead Code Removal**: Removed unused `GlobeRenderer::tick()`, `Utils::randomColorVariation()`.
- [ ] **Batching: Unified Line Renderer**: Merge `Pin`, `Marker`, and `IntroLine` renderers into a single vertex buffer to reduce draw calls.
- [ ] **Performance: GPU Instancing**: Use `drawInstanced` for satellites and pin heads to move the drawing loop from CPU to GPU.
- [ ] **Efficiency: Worker-Thread Projection**: Move 3D-to-2D label projection off the main thread to prevent UI stutter with high pin counts.
- [ ] **Stability: Triple Buffering**: Implement a buffer rotation strategy for frequently updated dynamic data (smoke, animated lines).

## Port Complete! 🚀
The `encom-globe` port to C++/Qt Quick is now feature-complete and optimized. 
Architecture uses pure RHI for custom graphics and native QML for UI/Labels.
