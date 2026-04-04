#include "Globe.h"
#include "GlobeRenderer.h"
#include "SatelliteRenderer.h"
#include "PinRenderer.h"
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
    update();
}

void Globe::addMarker(qreal lat, qreal lon, const QString& label, bool connected)
{
    Q_UNUSED(lat) Q_UNUSED(lon) Q_UNUSED(label) Q_UNUSED(connected)
    // TODO: Implement marker adding via renderer
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
        m_geometryChanged = false;
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
    
    // Animate pin progress
    bool pinsAnimating = false;
    for (auto& pin : m_pins) {
        if (pin.progress < 1.0f) {
            // Elastic out animation or simple lerp
            pin.progress += 0.016f; // rough 60fps step
            if (pin.progress > 1.0f) pin.progress = 1.0f;
            pinsAnimating = true;
            m_pinsChanged = true;
        }
    }
    
    if (m_pinsChanged) {
        pinNode->setPins(m_pins);
        m_pinsChanged = false;
    }
    
    // Schedule next frame for animation
    if (pinsAnimating) {
        update();
    }
    update(); // Since globe rotates anyway, keep updating
    
    return root;
}
