# CPU Optimization Summary

During the optimization phase of the Encom Globe port, while GPU batching reduced draw calls, the most significant CPU overhead was caused by hidden per-frame operations running at 60+ Hz. The following three major bottlenecks were identified and resolved:

### 1. Eliminating Per-Frame Trigonometry (XYZ Precomputation)
**Problem:** `Utils::latLonToXYZ()` computes expensive `std::sin` and `std::cos` operations. It was being called on every single vertex for satellites, pins, and markers during the per-frame `collectBillboards` and `collectLines` aggregation routines.
**Solution:** Added `basePos`, `pos`, and `endPos` cache vectors to the item data structures (`SatelliteData`, `PinData`, `MarkerData`). Trigonometry is now executed exactly *once* upon item insertion. Per-frame positioning for animations was simplified to extremely fast linear vector interpolation (`basePos + (endPos - basePos) * progress`).

### 2. Eliminating QML Engine Thrashing (Label Object Pool)
**Problem:** The 3D-to-2D label projection originally generated a new `QVariantList` mapped to a QML `Repeater` model. Because coordinates update continuously as the globe rotates, emitting `pinLabelsChanged()` every frame forced the QML Engine to completely destroy and re-instantiate every single `Text` delegate 60 times a second.
**Solution:** Replaced the `QVariantList` with a `QQmlListProperty<QObject>` backed by a dedicated `GlobeLabel` QObject pool. The main loop now safely updates `x`, `y`, `opacity`, and `text` properties directly on existing instances (synchronized via `QQuickWindow::afterAnimating`). This exploits Qt's ultra-fast property bindings and completely eradicates delegate reallocation overhead.

### 3. Eliminated `Repeater` Redraws
**Problem:** The QML repeater rebuilt all of its labels every time the array size changed, but QVariantLists always completely signal a structure change on modification.
**Solution:** `QQmlListProperty` with persistent instances inherently prevents standard list models from fully redrawing static arrays and fully enables the `NOTIFY` signals of `x` and `y` coordinates.
