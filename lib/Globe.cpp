#include "Globe.h"
#include "GlobeRenderer.h"
#include "SatelliteRenderer.h"
#include "PinRenderer.h"
#include "MarkerRenderer.h"
#include "SmokeRenderer.h"
#include "IntroLinesRenderer.h"
#include "Utils.h"
#include <QFile>
#include <QJsonDocument>
#include <QSGNode>
#include <QQuickWindow>

Globe::Globe(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::AllButtons);
    
    m_elapsed.start();
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

void Globe::setPinColor(const QString& color)
{
    if (m_pinColor != color) {
        m_pinColor = color;
        emit pinColorChanged();
        update();
    }
}

void Globe::setMarkerColor(const QString& color)
{
    if (m_markerColor != color) {
        m_markerColor = color;
        emit markerColorChanged();
        update();
    }
}

void Globe::setSatelliteColor(const QString& color)
{
    if (m_satelliteColor != color) {
        m_satelliteColor = color;
        for (auto& sat : m_satellites)
            sat.coreColor = QColor(color);
        m_satellitesChanged = true;
        emit satelliteColorChanged();
        update();
    }
}

void Globe::setIntroLinesColor(const QString& color)
{
    if (m_introLinesColor != color) {
        m_introLinesColor = color;
        emit introLinesColorChanged();
        update();
    }
}

void Globe::setMarkerSize(qreal size)
{
    if (!qFuzzyCompare(m_markerSize, size)) {
        m_markerSize = size;
        emit markerSizeChanged();
        update();
    }
}

void Globe::setRotationOffset(qreal offset)
{
    if (!qFuzzyCompare(m_rotationOffset, offset)) {
        m_rotationOffset = offset;
        emit rotationOffsetChanged();
        update();
    }
}

void Globe::setPinHeadSize(qreal size)
{
    if (!qFuzzyCompare(m_pinHeadSize, size)) {
        m_pinHeadSize = size;
        emit pinHeadSizeChanged();
        update();
    }
}

void Globe::setShowLabels(bool show)
{
    if (m_showLabels != show) {
        m_showLabels = show;
        emit showLabelsChanged();
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
    pin.altitude = 1.1f; 
    pin.text = label;
    pin.progress = 0.0f;
    pin.color = QColor(m_pinColor);
    
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
    mk.color = QColor(m_markerColor);
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
    sat.coreColor = QColor(m_satelliteColor);
    sat.shieldColor = QColor("#FFFFFF");
    sat.size = 1.0f;
    sat.progress = 0.0f;
    
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

void Globe::updateFrameData()
{
    if (m_startTime == -1) {
        m_startTime = m_elapsed.elapsed();
        m_lastFrameTime = m_startTime;
    }

    const qint64 currentTimeMs = m_elapsed.elapsed();
    m_frame.timeMs = currentTimeMs - m_startTime;
    
    if (m_lastFrameTime == 0) {
        m_frame.dt = 0.016f;
    } else {
        m_frame.dt = static_cast<float>(currentTimeMs - m_lastFrameTime) / 1000.0f;
    }
    m_lastFrameTime = currentTimeMs;

    // View State Calculation (Once per frame)
    m_frame.cameraAngle = static_cast<float>(m_rotationOffset) + static_cast<float>(M_PI) + (2.0f * M_PI * m_frame.timeMs) / m_dayLength;
    m_frame.cameraDist = Utils::CAMERA_DISTANCE / static_cast<float>(m_scale);
    
    m_frame.cameraPos = QVector3D(m_frame.cameraDist * std::cos(m_frame.cameraAngle), 
                                  m_viewAngle * m_frame.cameraDist, 
                                  m_frame.cameraDist * std::sin(m_frame.cameraAngle));
    m_frame.viewDir = m_frame.cameraPos.normalized();
    
    m_frame.view.setToIdentity();
    m_frame.view.lookAt(m_frame.cameraPos, QVector3D(0, 0, 0), QVector3D(0, 1, 0));
    
    const float aspect = width() > 0 && height() > 0 ? static_cast<float>(width() / height()) : 1.0f;
    m_frame.projection.setToIdentity();
    m_frame.projection.perspective(50.0f, aspect, 1.0f, m_frame.cameraDist + 500);
    
    m_frame.mvp = m_frame.projection * m_frame.view;
}

void Globe::updateState()
{
    // Frame logic (Animations + Label Projections)
    // updateFrameData() must have been called by updatePaintNode() first
    
    QVariantList newLabels;
    
    // Animate satellite intro growth
    for (auto& sat : m_satellites) {
        if (sat.progress < 1.0f) {
            sat.progress += m_frame.dt / 0.3f; 
            if (sat.progress > 1.0f) sat.progress = 1.0f;
            m_satellitesChanged = true;
        }
    }
    
    // Animate pins and project labels
    for (auto& pinItem : m_pins) {
        if (pinItem.progress < 1.0f) {
            pinItem.progress += m_frame.dt / 1.0f;
            if (pinItem.progress > 1.0f) pinItem.progress = 1.0f;
            m_pinsChanged = true;
        }
        
        if (!pinItem.text.isEmpty() && pinItem.progress > 0.3f) {
            float animatedProgress = Utils::elasticOut(pinItem.progress);
            float currentAltitude = 1.0f + (pinItem.altitude - 1.0f) * animatedProgress + 0.05f;
            QVector3D pos = Utils::latLonToXYZ(pinItem.lat, pinItem.lon, Utils::GLOBE_RADIUS * currentAltitude);
            
            QVector4D clipPos = m_frame.mvp * QVector4D(pos, 1.0f);
            if (clipPos.w() > 0.0f) {
                float ndcX = clipPos.x() / clipPos.w();
                float ndcY = clipPos.y() / clipPos.w();
                QVector3D surfacePos = Utils::latLonToXYZ(pinItem.lat, pinItem.lon, Utils::GLOBE_RADIUS);
                if (QVector3D::dotProduct(surfacePos.normalized(), m_frame.viewDir) > -0.2f) {
                    QVariantMap map;
                    map["x"] = (ndcX + 1.0f) * 0.5f * width();
                    map["y"] = (1.0f - ndcY) * 0.5f * height();
                    map["text"] = pinItem.text;
                    map["opacity"] = (pinItem.progress - 0.3f) / 0.7f;
                    newLabels.append(map);
                }
            }
        }
    }
    
    // Animate markers and project labels
    for (size_t mi = 0; mi < m_markers.size(); ++mi) {
        auto& mk = m_markers[mi];
        bool changed = false;
        
        bool canAnimate = true;
        if (mk.previousIndex >= 0 && mk.previousIndex < (int)m_markers.size()) {
            canAnimate = m_markers[mk.previousIndex].lineProgress >= 1.0f;
        }
        
        if (canAnimate && mk.lineProgress < 1.0f) {
            mk.lineProgress += m_frame.dt / 2.0f;
            if (mk.lineProgress > 1.0f) mk.lineProgress = 1.0f;
            changed = true;
        }
        
        float markerDelay = (mk.previousIndex >= 0) ? 0.8f : 0.0f;
        if (mk.lineProgress > markerDelay && mk.markerProgress < 1.0f) {
            mk.markerProgress += m_frame.dt / 0.5f;
            if (mk.markerProgress > 1.0f) mk.markerProgress = 1.0f;
            changed = true;
        }
        
        if (!mk.text.isEmpty() && mk.markerProgress > 0.1f) {
            QVector3D pos = Utils::latLonToXYZ(mk.lat, mk.lon, Utils::GLOBE_RADIUS * mk.altitude + 30.0f);
            QVector4D clipPos = m_frame.mvp * QVector4D(pos, 1.0f);
            if (clipPos.w() > 0.0f) {
                float ndcX = clipPos.x() / clipPos.w();
                float ndcY = clipPos.y() / clipPos.w();
                QVector3D surfacePos = Utils::latLonToXYZ(mk.lat, mk.lon, Utils::GLOBE_RADIUS);
                if (QVector3D::dotProduct(surfacePos.normalized(), m_frame.viewDir) > -0.2f) {
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
}

QSGNode* Globe::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data)
{
    QSGNode* root = oldNode;
    if (!root) {
        root = new QSGNode(); 
    }
    
    // 1. Calculate shared frame data ONCE
    updateFrameData();

    // 2. Perform UI logic (animations + labels) using that data
    updateState();

    // Get or create renderers
    GlobeRenderer* globeNode = static_cast<GlobeRenderer*>(
        root->childCount() > 0 ? root->childAtIndex(0) : nullptr);
    if (!globeNode) {
        globeNode = new GlobeRenderer();
        root->appendChildNode(globeNode);
    }
    
    SatelliteRenderer* satNode = static_cast<SatelliteRenderer*>(
        root->childCount() > 1 ? root->childAtIndex(1) : nullptr);
    if (!satNode) {
        satNode = new SatelliteRenderer();
        root->appendChildNode(satNode);
    }
    
    PinRenderer* pinNode = static_cast<PinRenderer*>(
        root->childCount() > 2 ? root->childAtIndex(2) : nullptr);
    if (!pinNode) {
        pinNode = new PinRenderer();
        root->appendChildNode(pinNode);
    }
    
    MarkerRenderer* markerNode = static_cast<MarkerRenderer*>(
        root->childCount() > 3 ? root->childAtIndex(3) : nullptr);
    if (!markerNode) {
        markerNode = new MarkerRenderer();
        root->appendChildNode(markerNode);
    }
    
    SmokeRenderer* smokeNode = static_cast<SmokeRenderer*>(
        root->childCount() > 4 ? root->childAtIndex(4) : nullptr);
    if (!smokeNode) {
        smokeNode = new SmokeRenderer();
        root->appendChildNode(smokeNode);
    }
    
    IntroLinesRenderer* introLinesNode = static_cast<IntroLinesRenderer*>(
        root->childCount() > 5 ? root->childAtIndex(5) : nullptr);
    if (!introLinesNode) {
        introLinesNode = new IntroLinesRenderer();
        root->appendChildNode(introLinesNode);
    }
    
    // Calculate viewport pixel rect
    const qreal dpr = window()->devicePixelRatio();
    const QRectF screenRect = mapRectToScene(QRectF(0, 0, width(), height()));
    const QRect pixelRect = QRect(
        qRound(screenRect.x() * dpr),
        qRound(screenRect.y() * dpr),
        qRound(screenRect.width() * dpr),
        qRound(screenRect.height() * dpr)
    );
    
    // Distribute shared data to all nodes
    globeNode->setMVP(m_frame.mvp);
    globeNode->setViewDir(m_frame.viewDir);
    globeNode->setViewportRect(pixelRect);
    globeNode->setCurrentTime(static_cast<float>(m_frame.timeMs));
    globeNode->setIntroDuration(static_cast<float>(m_introDuration));
    globeNode->setIntroAltitude(1.10f);
    globeNode->setBaseColor(m_baseColor);
    if (m_tilesLoaded) { globeNode->setTileData(m_tileData); m_tilesLoaded = false; }
    if (m_geometryChanged) globeNode->setSize(QSizeF(width(), height()));
    
    satNode->setMVP(m_frame.mvp);
    satNode->setViewDir(m_frame.viewDir);
    satNode->setViewportRect(pixelRect);
    satNode->setTime(static_cast<float>(m_frame.timeMs));
    satNode->setCameraPosition(m_frame.cameraPos);
    if (m_geometryChanged) satNode->setSize(QSizeF(width(), height()));
    if (m_satellitesChanged) { satNode->setSatellites(m_satellites); m_satellitesChanged = false; }
    
    pinNode->setMVP(m_frame.mvp);
    pinNode->setViewDir(m_frame.viewDir);
    pinNode->setCameraPosition(m_frame.cameraPos);
    pinNode->setHeadScale(static_cast<float>(m_pinHeadSize));
    pinNode->setViewportRect(pixelRect);
    if (m_geometryChanged) pinNode->setSize(QSizeF(width(), height()));
    if (m_pinsChanged) { pinNode->setPins(m_pins); m_pinsChanged = false; }
    
    markerNode->setMVP(m_frame.mvp);
    markerNode->setCameraPosition(m_frame.cameraPos);
    markerNode->setViewDir(m_frame.viewDir);
    markerNode->setSpriteScale(static_cast<float>(m_markerSize));
    markerNode->setViewportRect(pixelRect);
    if (m_geometryChanged) markerNode->setSize(QSizeF(width(), height()));
    if (m_markersChanged) { markerNode->setMarkers(m_markers); m_markersChanged = false; }
    
    smokeNode->setMVP(m_frame.mvp);
    smokeNode->setViewDir(m_frame.viewDir);
    smokeNode->setTime(static_cast<float>(m_frame.timeMs));
    smokeNode->setViewportRect(pixelRect);
    for (const auto& s : m_newSmokeSources) smokeNode->addSource(s.lat, s.lon, s.alt);
    m_newSmokeSources.clear();
    
    introLinesNode->setMVP(m_frame.mvp);
    introLinesNode->setViewDir(m_frame.viewDir);
    introLinesNode->setTime(static_cast<float>(m_frame.timeMs));
    introLinesNode->setDuration(static_cast<float>(m_introDuration));
    introLinesNode->setViewportRect(pixelRect);
    if (m_geometryChanged) introLinesNode->setSize(QSizeF(width(), height()));
    
    if (m_geometryChanged) m_geometryChanged = false;
    
    update(); // Request next frame from SceneGraph
    return root;
}
