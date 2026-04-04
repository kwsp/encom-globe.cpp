#include "Globe.h"
#include "GlobeRenderer.h"
#include "SatelliteRenderer.h"
#include "PinRenderer.h"
#include "MarkerRenderer.h"
#include "SmokeRenderer.h"
#include "Utils.h"
#include <QFile>
#include <QJsonDocument>
#include <QSGNode>

Globe::Globe(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::AllButtons);
    
    // Start animation timer
    m_elapsed.start();
    m_startTime = m_elapsed.elapsed();
    
    // Logic timer for QML property updates
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &Globe::updateState);
    m_updateTimer->start(16);
}

Globe::~Globe() = default;

void Globe::componentComplete()
{
    QQuickItem::componentComplete();
    loadTileData();
}

void Globe::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        m_geometryChanged = true;
        update();
    }
}

void Globe::loadTileData()
{
    QFile file(":/qt/qml/EncomGlobe/grid.json");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        m_tileData = doc.object();
        m_tilesLoaded = true;
    } else {
        qWarning() << "Failed to load tile data from grid.json";
    }
}

void Globe::setDayLength(qreal len)
{
    if (!qFuzzyCompare(m_dayLength, len)) {
        m_dayLength = len;
        emit dayLengthChanged();
        update();
    }
}

void Globe::setScale(qreal s)
{
    if (!qFuzzyCompare(m_scale, s)) {
        m_scale = s;
        emit scaleChanged();
        update();
    }
}

void Globe::setViewAngle(qreal angle)
{
    if (!qFuzzyCompare(m_viewAngle, angle)) {
        m_viewAngle = angle;
        emit viewAngleChanged();
        update();
    }
}

void Globe::setBaseColor(const QString& color)
{
    if (m_baseColor != color) {
        m_baseColor = color;
        emit baseColorChanged();
        update();
    }
}

void Globe::setIntroDuration(qreal duration)
{
    if (!qFuzzyCompare(m_introDuration, duration)) {
        m_introDuration = duration;
        emit introDurationChanged();
        update();
    }
}

void Globe::addPin(qreal lat, qreal lon, const QString& label)
{
    PinData pin;
    pin.lat = static_cast<float>(lat);
    pin.lon = static_cast<float>(lon);
    pin.altitude = 1.1f; // Target altitude (10% above surface)
    pin.text = label;
    pin.progress = 0.0f; // Start animation at 0
    pin.color = QColor("#8FD8D8");
    
    m_pins.push_back(pin);
    m_pinsChanged = true;
    m_newSmokeSources.push_back({(float)lat, (float)lon, 1.1f});
    update();
}

void Globe::addMarker(qreal lat, qreal lon, const QString& label, bool connected)
{
    MarkerData mk;
    mk.lat = static_cast<float>(lat);
    mk.lon = static_cast<float>(lon);
    mk.altitude = 1.2f;
    mk.text = label;
    mk.color = QColor("#FFCC00");
    mk.lineProgress = 0.0f;
    mk.markerProgress = 0.0f;
    
    if (connected && !m_markers.empty()) {
        mk.previousIndex = static_cast<int>(m_markers.size()) - 1;
    } else {
        mk.previousIndex = -1;
    }
    
    m_markers.push_back(mk);
    m_markersChanged = true;
    m_newSmokeSources.push_back({(float)lat, (float)lon, 1.2f});
    update();
}

void Globe::addSatellite(qreal lat, qreal lon, qreal altitude)
{
    SatelliteData sat;
    sat.lat = static_cast<float>(lat);
    sat.lon = static_cast<float>(lon);
    sat.altitude = static_cast<float>(altitude);
    sat.waveColor = QColor("#FFFFFF");
    sat.coreColor = QColor("#FF0000");
    sat.shieldColor = QColor("#FFFFFF");
    sat.size = 1.0f;
    
    m_satellites.push_back(sat);
    m_satellitesChanged = true;
    update();
}

void Globe::clearSatellites()
{
    m_satellites.clear();
    m_satellitesChanged = true;
    update();
}

void Globe::scheduleUpdate()
{
    update();
}

void Globe::updateState()
{
    // Calculate MVP here so we can project labels to 2D
    const qint64 currentTime = m_elapsed.elapsed() - m_startTime;
    const float cameraAngle = (2.0f * M_PI * currentTime) / m_dayLength;
    const float dist = Utils::CAMERA_DISTANCE / static_cast<float>(m_scale);
    
    QVector3D cameraPos(dist * std::cos(cameraAngle), 
                         m_viewAngle * dist, 
                         dist * std::sin(cameraAngle));
    
    QMatrix4x4 view;
    view.lookAt(cameraPos, QVector3D(0, 0, 0), QVector3D(0, 1, 0));
    
    QMatrix4x4 projection;
    const float aspect = width() > 0 && height() > 0 ? static_cast<float>(width() / height()) : 1.0f;
    projection.perspective(50.0f, aspect, 1.0f, dist + 500);
    
    QMatrix4x4 viewProj = projection * view;
    
    QVariantList newLabels;
    
    // Animate pin progress and create label data
    for (auto& pin : m_pins) {
        if (pin.progress < 1.0f) {
            pin.progress += 0.016f;
            if (pin.progress > 1.0f) pin.progress = 1.0f;
            m_pinsChanged = true;
        }
        
        if (!pin.text.isEmpty() && pin.progress > 0.3f) {
            float animatedProgress = Utils::elasticOut(pin.progress);
            float currentAltitude = 1.0f + (pin.altitude - 1.0f) * animatedProgress + 0.05f;
            QVector3D pos = Utils::latLonToXYZ(pin.lat, pin.lon, Utils::GLOBE_RADIUS * currentAltitude);
            
            QVector4D clipPos = viewProj * QVector4D(pos, 1.0f);
            if (clipPos.w() > 0.0f) {
                float ndcX = clipPos.x() / clipPos.w();
                float ndcY = clipPos.y() / clipPos.w();
                QVector3D surfacePos = Utils::latLonToXYZ(pin.lat, pin.lon, Utils::GLOBE_RADIUS);
                if (QVector3D::dotProduct(surfacePos.normalized(), cameraPos.normalized()) > -0.2f) {
                    QVariantMap map;
                    map["x"] = (ndcX + 1.0f) * 0.5f * width();
                    map["y"] = (1.0f - ndcY) * 0.5f * height();
                    map["text"] = pin.text;
                    map["opacity"] = (pin.progress - 0.3f) / 0.7f;
                    newLabels.append(map);
                }
            }
        }
    }
    
    // Animate markers and add labels
    for (auto& mk : m_markers) {
        bool changed = false;
        if (mk.lineProgress < 1.0f) {
            mk.lineProgress += 0.008f;
            if (mk.lineProgress > 1.0f) mk.lineProgress = 1.0f;
            changed = true;
        }
        float markerDelay = (mk.previousIndex >= 0) ? 0.8f : 0.0f;
        if (mk.lineProgress > markerDelay && mk.markerProgress < 1.0f) {
            mk.markerProgress += 0.02f;
            if (mk.markerProgress > 1.0f) mk.markerProgress = 1.0f;
            changed = true;
        }
        
        if (!mk.text.isEmpty() && mk.markerProgress > 0.1f) {
            QVector3D pos = Utils::latLonToXYZ(mk.lat, mk.lon, Utils::GLOBE_RADIUS * mk.altitude + 30.0f);
            QVector4D clipPos = viewProj * QVector4D(pos, 1.0f);
            if (clipPos.w() > 0.0f) {
                float ndcX = clipPos.x() / clipPos.w();
                float ndcY = clipPos.y() / clipPos.w();
                QVector3D surfacePos = Utils::latLonToXYZ(mk.lat, mk.lon, Utils::GLOBE_RADIUS);
                if (QVector3D::dotProduct(surfacePos.normalized(), cameraPos.normalized()) > -0.2f) {
                    QVariantMap map;
                    map["x"] = (ndcX + 1.0f) * 0.5f * width();
                    map["y"] = (1.0f - ndcY) * 0.5f * height();
                    map["text"] = mk.text;
                    map["opacity"] = mk.markerProgress;
                    newLabels.append(map);
                }
            }
        }
        
        if (changed) {
            m_markersChanged = true;
        }
    }
    
    if (newLabels != m_pinLabels) {
        m_pinLabels = newLabels;
        emit pinLabelsChanged();
    }
    
    update(); // schedule render
}

QSGNode* Globe::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data)
{
    // We use a simple structure:
    // - Root node (transform node for any item-level transforms)
    //   - GlobeRenderer (the globe tiles)
    //   - SatelliteRenderer (satellites as child node)
    
    QSGNode* root = oldNode;
    
    if (!root) {
        root = new QSGNode();  // Root transform node
    }
    
    // Get or create globe renderer (first child)
    GlobeRenderer* globeNode = static_cast<GlobeRenderer*>(root->firstChild());
    if (!globeNode) {
        globeNode = new GlobeRenderer();
        root->appendChildNode(globeNode);
    }
    
    // Get or create satellite renderer (second child)
    SatelliteRenderer* satNode = static_cast<SatelliteRenderer*>(
        root->childCount() > 1 ? root->childAtIndex(1) : nullptr);
    if (!satNode) {
        satNode = new SatelliteRenderer();
        root->appendChildNode(satNode);
    }
    
    // Get or create pin renderer (third child)
    PinRenderer* pinNode = static_cast<PinRenderer*>(
        root->childCount() > 2 ? root->childAtIndex(2) : nullptr);
    if (!pinNode) {
        pinNode = new PinRenderer();
        root->appendChildNode(pinNode);
    }
    
    // Get or create marker renderer (fourth child)
    MarkerRenderer* markerNode = static_cast<MarkerRenderer*>(
        root->childCount() > 3 ? root->childAtIndex(3) : nullptr);
    if (!markerNode) {
        markerNode = new MarkerRenderer();
        root->appendChildNode(markerNode);
    }
    
    // Get or create smoke renderer (fifth child)
    SmokeRenderer* smokeNode = static_cast<SmokeRenderer*>(
        root->childCount() > 4 ? root->childAtIndex(4) : nullptr);
    if (!smokeNode) {
        smokeNode = new SmokeRenderer();
        root->appendChildNode(smokeNode);
    }
    
    // Calculate camera and matrices
    const qint64 currentTime = m_elapsed.elapsed() - m_startTime;
    
    // Calculate camera position (orbiting)
    const float cameraAngle = (2.0f * M_PI * currentTime) / m_dayLength;
    const float dist = Utils::CAMERA_DISTANCE / static_cast<float>(m_scale);
    
    QVector3D cameraPos(dist * std::cos(cameraAngle), 
                         m_viewAngle * dist, 
                         dist * std::sin(cameraAngle));
    
    QMatrix4x4 view;
    view.lookAt(
        cameraPos,
        QVector3D(0, 0, 0),
        QVector3D(0, 1, 0)
    );
    
    // Use the item's size for aspect ratio
    QMatrix4x4 projection;
    const float aspect = width() > 0 && height() > 0 ? static_cast<float>(width() / height()) : 1.0f;
    projection.perspective(50.0f, aspect, 1.0f, dist + 500);
    
    QMatrix4x4 mvp = projection * view;
    
    // Update globe renderer
    globeNode->setMVP(mvp);
    globeNode->setCurrentTime(static_cast<float>(currentTime));
    globeNode->setIntroDuration(static_cast<float>(m_introDuration));
    globeNode->setIntroAltitude(1.10f);
    globeNode->setBaseColor(m_baseColor);
    
    if (m_tilesLoaded) {
        globeNode->setTileData(m_tileData);
        m_tilesLoaded = false;
    }
    
    if (m_geometryChanged) {
        globeNode->setSize(QSizeF(width(), height()));
    }
    
    // Update satellite renderer
    satNode->setMVP(mvp);
    satNode->setView(view);
    satNode->setTime(static_cast<float>(currentTime));
    satNode->setCameraPosition(cameraPos);
    satNode->setCameraAngle(cameraAngle);
    
    if (m_geometryChanged) {
        satNode->setSize(QSizeF(width(), height()));
    }
    
    if (m_satellitesChanged) {
        satNode->setSatellites(m_satellites);
        m_satellitesChanged = false;
    }
    
    // Update pin renderer
    pinNode->setMVP(mvp);
    
    if (m_geometryChanged) {
        pinNode->setSize(QSizeF(width(), height()));
    }
    
    // Animate pin progress is now handled in updateState()
    
    if (m_pinsChanged) {
        pinNode->setPins(m_pins);
        m_pinsChanged = false;
    }
    
    // Update marker renderer
    markerNode->setMVP(mvp);
    markerNode->setCameraPosition(cameraPos);
    
    if (m_geometryChanged) {
        markerNode->setSize(QSizeF(width(), height()));
    }
    
    if (m_markersChanged) {
        markerNode->setMarkers(m_markers);
        m_markersChanged = false;
    }
    
    // Update smoke renderer
    smokeNode->setMVP(mvp);
    smokeNode->setTime(static_cast<float>(currentTime));
    
    for (const auto& s : m_newSmokeSources) {
        smokeNode->addSource(s.lat, s.lon, s.alt);
    }
    m_newSmokeSources.clear();
    
    // Reset geometry changed flag at the end
    if (m_geometryChanged) {
        m_geometryChanged = false;
    }
    
    // Schedule next frame for animation
    update(); // Since globe rotates anyway, keep updating
    
    return root;
}
