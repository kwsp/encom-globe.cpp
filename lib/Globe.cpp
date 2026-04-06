#include "Globe.h"
#include "GlobeRenderer.h"
#include "IntroLinesRenderer.h"
#include "MarkerRenderer.h"
#include "PinRenderer.h"
#include "SatelliteRenderer.h"
#include "SmokeRenderer.h"
#include "Utils.h"
#include <QFile>
#include <QJsonDocument>
#include <QQuickWindow>
#include <QSGNode>
#include <numbers>

Globe::Globe(QQuickItem *parent) : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::AllButtons);
}

Globe::~Globe() {
    qDeleteAll(m_pinLabelsList);
}

void Globe::componentComplete() {
    QQuickItem::componentComplete();
    loadTileData();
}

void Globe::itemChange(ItemChange change, const ItemChangeData &data) {
    QQuickItem::itemChange(change, data);
    if (change == ItemSceneChange && data.window) {
        connect(data.window, &QQuickWindow::afterAnimating, this, &Globe::syncAnimation);
    }
}

void Globe::syncAnimation() {
    updateFrameData();
    updateState();
    update();
}

void Globe::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        m_geometryChanged = true;
        update();
    }
}

void Globe::loadTileData() {
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

void Globe::setDayLength(qreal len) {
    if (!qFuzzyCompare(m_dayLength, len)) {
        m_dayLength = len;
        emit dayLengthChanged();
        update();
    }
}

void Globe::setScale(qreal s) {
    if (!qFuzzyCompare(m_scale, s)) {
        m_scale = s;
        emit scaleChanged();
        update();
    }
}

void Globe::setViewAngle(qreal angle) {
    if (!qFuzzyCompare(m_viewAngle, angle)) {
        m_viewAngle = angle;
        emit viewAngleChanged();
        update();
    }
}

void Globe::setBaseColor(const QString &color) {
    if (m_baseColor != color) {
        m_baseColor = color;
        emit baseColorChanged();
        update();
    }
}

void Globe::setPinColor(const QString &color) {
    if (m_pinColor != color) {
        m_pinColor = color;
        emit pinColorChanged();
        update();
    }
}

void Globe::setMarkerColor(const QString &color) {
    if (m_markerColor != color) {
        m_markerColor = color;
        emit markerColorChanged();
        update();
    }
}

void Globe::setSatelliteColor(const QString &color) {
    if (m_satelliteColor != color) {
        m_satelliteColor = color;
        for (auto &sat : m_satellites)
            sat.coreColor = QColor(color);
        m_satellitesChanged = true;
        emit satelliteColorChanged();
        update();
    }
}

void Globe::setIntroLinesColor(const QString &color) {
    if (m_introLinesColor != color) {
        m_introLinesColor = color;
        emit introLinesColorChanged();
        update();
    }
}

void Globe::setMarkerSize(qreal size) {
    if (!qFuzzyCompare(m_markerSize, size)) {
        m_markerSize = size;
        emit markerSizeChanged();
        update();
    }
}

void Globe::setRotationOffset(qreal offset) {
    if (!qFuzzyCompare(m_rotationOffset, offset)) {
        m_rotationOffset = offset;
        emit rotationOffsetChanged();
        update();
    }
}

void Globe::setPinHeadSize(qreal size) {
    if (!qFuzzyCompare(m_pinHeadSize, size)) {
        m_pinHeadSize = size;
        emit pinHeadSizeChanged();
        update();
    }
}

void Globe::setShowLabels(bool show) {
    if (m_showLabels != show) {
        m_showLabels = show;
        emit showLabelsChanged();
        update();
    }
}

QQmlListProperty<QObject> Globe::pinLabels() {
    return QQmlListProperty<QObject>(this, &m_pinLabelsList);
}

void Globe::setIntroDuration(qreal duration) {
    if (!qFuzzyCompare(m_introDuration, duration)) {
        m_introDuration = duration;
        emit introDurationChanged();
        update();
    }
}

void Globe::addPin(qreal lat, qreal lon, const QString &label) {
    PinData pin;
    pin.lat = static_cast<float>(lat);
    pin.lon = static_cast<float>(lon);
    pin.altitude = 1.1F;
    pin.text = label;
    pin.progress = 0.0F;
    pin.color = QColor(m_pinColor);
    pin.basePos = Utils::latLonToXYZ(pin.lat, pin.lon, Utils::GLOBE_RADIUS);
    pin.endPos = Utils::latLonToXYZ(pin.lat, pin.lon, Utils::GLOBE_RADIUS * pin.altitude);

    m_pins.push_back(pin);
    m_pinsChanged = true;
    m_newSmokeSources.push_back({(float)lat, (float)lon, 1.1F});
    update();
}

void Globe::addMarker(qreal lat, qreal lon, const QString &label, bool connected) {
    MarkerData mk;
    mk.lat = static_cast<float>(lat);
    mk.lon = static_cast<float>(lon);
    mk.altitude = 1.2F;
    mk.text = label;
    mk.color = QColor(m_markerColor);
    mk.lineProgress = 0.0F;
    mk.markerProgress = 0.0F;
    mk.basePos = Utils::latLonToXYZ(mk.lat, mk.lon, Utils::GLOBE_RADIUS);
    mk.pos = Utils::latLonToXYZ(mk.lat, mk.lon, Utils::GLOBE_RADIUS * mk.altitude);

    if (connected && !m_markers.empty()) {
        mk.previousIndex = static_cast<int>(m_markers.size()) - 1;
    } else {
        mk.previousIndex = -1;
    }

    m_markers.push_back(mk);
    m_markersChanged = true;
    m_newSmokeSources.push_back({(float)lat, (float)lon, 1.2F});
    update();
}

void Globe::addSatellite(qreal lat, qreal lon, qreal altitude) {
    SatelliteData sat;
    sat.lat = static_cast<float>(lat);
    sat.lon = static_cast<float>(lon);
    sat.altitude = static_cast<float>(altitude);
    sat.waveColor = QColor("#FFFFFF");
    sat.coreColor = QColor(m_satelliteColor);
    sat.shieldColor = QColor("#FFFFFF");
    sat.size = 1.0F;
    sat.progress = 0.0F;
    sat.pos = Utils::latLonToXYZ(sat.lat, sat.lon, Utils::GLOBE_RADIUS * sat.altitude);

    m_satellites.push_back(sat);
    m_satellitesChanged = true;
    update();
}

void Globe::clearSatellites() {
    m_satellites.clear();
    m_satellitesChanged = true;
    update();
}

void Globe::scheduleUpdate() {
    update();
}

void Globe::updateFrameData() {
    if (m_startTime == -1) {
        m_elapsed.start();
        m_startTime = 0;
        m_lastFrameTime = 0;
    }

    const qint64 currentTimeMs = m_elapsed.elapsed();
    m_frame.timeMs = static_cast<float>(currentTimeMs - m_startTime);
    m_frame.dt = static_cast<float>((currentTimeMs - m_lastFrameTime) / 1000.0);
    m_lastFrameTime = currentTimeMs;

    // View State Calculation (Once per frame)
    m_frame.cameraAngle = static_cast<float>(m_rotationOffset) + std::numbers::pi_v<float> +
                          ((2 * std::numbers::pi_v<float> * m_frame.timeMs) / m_dayLength);
    m_frame.cameraDist = Utils::CAMERA_DISTANCE / static_cast<float>(m_scale);

    m_frame.cameraPos = QVector3D(m_frame.cameraDist * std::cos(m_frame.cameraAngle),
                                  m_viewAngle * m_frame.cameraDist,
                                  m_frame.cameraDist * std::sin(m_frame.cameraAngle));
    m_frame.viewDir = m_frame.cameraPos.normalized();

    m_frame.view.setToIdentity();
    m_frame.view.lookAt(m_frame.cameraPos, QVector3D(0, 0, 0), QVector3D(0, 1, 0));

    const float aspect =
        width() > 0 && height() > 0 ? static_cast<float>(width() / height()) : 1.0F;
    m_frame.projection.setToIdentity();
    m_frame.projection.perspective(50.0F, aspect, 1.0F, m_frame.cameraDist + 500);

    m_frame.mvp = m_frame.projection * m_frame.view;
}

void Globe::updateState() {
    // Pre-calculate variables
    const float halfW = width() * 0.5F;
    const float halfH = height() * 0.5F;

    int labelIdx = 0;

    // Animate satellite intro growth
    for (auto &sat : m_satellites) {
        if (sat.progress < 1.0F) {
            sat.progress += m_frame.dt / 0.3F;
            if (sat.progress > 1.0F)
                sat.progress = 1.0F;
            m_satellitesChanged = true;
        }
    }

    // Animate pins and project labels
    for (auto &pinItem : m_pins) {
        if (pinItem.progress < 1.0F) {
            pinItem.progress += m_frame.dt / 1.0F;
            if (pinItem.progress > 1.0F)
                pinItem.progress = 1.0F;
            m_pinsChanged = true;
        }

        if (!pinItem.text.isEmpty() && pinItem.progress > 0.3F) {
            float animatedProgress = Utils::elasticOut(pinItem.progress);
            QVector3D pos = pinItem.basePos + (pinItem.endPos - pinItem.basePos) * animatedProgress;

            QVector4D clipPos = m_frame.mvp * QVector4D(pos, 1.0F);
            if (clipPos.w() > 0.0F) {
                float ndcX = clipPos.x() / clipPos.w();
                float ndcY = clipPos.y() / clipPos.w();
                if (QVector3D::dotProduct(pinItem.basePos / 100.0F, m_frame.viewDir) > -0.2F) {

                    GlobeLabel *lbl;
                    if (labelIdx < m_pinLabelsList.size()) {
                        lbl = static_cast<GlobeLabel *>(m_pinLabelsList[labelIdx]);
                    } else {
                        lbl = new GlobeLabel(this);
                        m_pinLabelsList.append(lbl);
                        emit pinLabelsChanged();
                    }
                    lbl->setX((ndcX + 1.0F) * halfW);
                    lbl->setY((1.0F - ndcY) * halfH);
                    lbl->setText(pinItem.text);
                    lbl->setOpacity((pinItem.progress - 0.3F) / 0.7F);
                    labelIdx++;
                }
            }
        }
    }

    // Animate markers and project labels
    for (size_t mi = 0; mi < m_markers.size(); ++mi) {
        auto &mk = m_markers[mi];
        bool changed = false;

        bool canAnimate = true;
        if (mk.previousIndex >= 0 && mk.previousIndex < (int)m_markers.size()) {
            canAnimate = m_markers[mk.previousIndex].lineProgress >= 1.0F;
        }

        if (canAnimate && mk.lineProgress < 1.0F) {
            mk.lineProgress += m_frame.dt / 2.0F;
            if (mk.lineProgress > 1.0F)
                mk.lineProgress = 1.0F;
            changed = true;
        }

        float markerDelay = (mk.previousIndex >= 0) ? 0.8F : 0.0F;
        if (mk.lineProgress > markerDelay && mk.markerProgress < 1.0F) {
            mk.markerProgress += m_frame.dt / 0.5F;
            if (mk.markerProgress > 1.0F)
                mk.markerProgress = 1.0F;
            changed = true;
        }

        if (!mk.text.isEmpty() && mk.markerProgress > 0.1F) {
            QVector3D pos = mk.pos + (mk.basePos / 100.0F) * 30.0F; // offset slightly out
            QVector4D clipPos = m_frame.mvp * QVector4D(pos, 1.0F);
            if (clipPos.w() > 0.0F) {
                float ndcX = clipPos.x() / clipPos.w();
                float ndcY = clipPos.y() / clipPos.w();
                if (QVector3D::dotProduct(mk.basePos / 100.0F, m_frame.viewDir) > -0.2F) {

                    GlobeLabel *lbl;
                    if (labelIdx < m_pinLabelsList.size()) {
                        lbl = static_cast<GlobeLabel *>(m_pinLabelsList[labelIdx]);
                    } else {
                        lbl = new GlobeLabel(this);
                        m_pinLabelsList.append(lbl);
                        emit pinLabelsChanged();
                    }
                    lbl->setX((ndcX + 1.0F) * halfW);
                    lbl->setY((1.0F - ndcY) * halfH);
                    lbl->setText(mk.text);
                    lbl->setOpacity(mk.markerProgress);
                    labelIdx++;
                }
            }
        }

        if (changed) {
            m_markersChanged = true;
        }
    }

    // Remove excess labels
    if (labelIdx < m_pinLabelsList.size()) {
        while (m_pinLabelsList.size() > labelIdx) {
            delete m_pinLabelsList.takeLast();
        }
        emit pinLabelsChanged();
    }
}

QSGNode *Globe::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) {
    QSGNode *root = oldNode;
    if (!root) {
        root = new QSGNode();
    }

    // Get or create renderers
    auto *globeNode =
        static_cast<GlobeRenderer *>(root->childCount() > 0 ? root->childAtIndex(0) : nullptr);
    if (!globeNode) {
        globeNode = new GlobeRenderer();
        root->appendChildNode(globeNode);
    }

    auto *satNode =
        static_cast<SatelliteRenderer *>(root->childCount() > 1 ? root->childAtIndex(1) : nullptr);
    if (!satNode) {
        satNode = new SatelliteRenderer();
        root->appendChildNode(satNode);
    }

    auto *pinNode =
        static_cast<PinRenderer *>(root->childCount() > 2 ? root->childAtIndex(2) : nullptr);
    if (!pinNode) {
        pinNode = new PinRenderer();
        root->appendChildNode(pinNode);
    }

    auto *markerNode =
        static_cast<MarkerRenderer *>(root->childCount() > 3 ? root->childAtIndex(3) : nullptr);
    if (!markerNode) {
        markerNode = new MarkerRenderer();
        root->appendChildNode(markerNode);
    }

    auto *smokeNode =
        static_cast<SmokeRenderer *>(root->childCount() > 4 ? root->childAtIndex(4) : nullptr);
    if (!smokeNode) {
        smokeNode = new SmokeRenderer();
        root->appendChildNode(smokeNode);
    }

    auto *introLinesNode =
        static_cast<IntroLinesRenderer *>(root->childCount() > 5 ? root->childAtIndex(5) : nullptr);
    if (!introLinesNode) {
        introLinesNode = new IntroLinesRenderer();
        root->appendChildNode(introLinesNode);
    }

    // Calculate viewport pixel rect
    const qreal dpr = window()->devicePixelRatio();
    const QRectF screenRect = mapRectToScene(QRectF(0, 0, width(), height()));
    const QRect pixelRect =
        QRect(qRound(screenRect.x() * dpr), qRound(screenRect.y() * dpr),
              qRound(screenRect.width() * dpr), qRound(screenRect.height() * dpr));

    // Distribute shared data to all nodes
    globeNode->setMVP(m_frame.mvp);
    globeNode->setViewDir(m_frame.viewDir);
    globeNode->setViewportRect(pixelRect);
    globeNode->setCurrentTime(static_cast<float>(m_frame.timeMs));
    globeNode->setIntroDuration(static_cast<float>(m_introDuration));
    globeNode->setIntroAltitude(1.10F);
    globeNode->setBaseColor(m_baseColor);
    if (m_tilesLoaded) {
        globeNode->setTileData(m_tileData);
        m_tilesLoaded = false;
    }
    if (m_geometryChanged)
        globeNode->setSize(QSizeF(width(), height()));

    satNode->setMVP(m_frame.mvp);
    satNode->setViewDir(m_frame.viewDir);
    satNode->setViewportRect(pixelRect);
    satNode->setTime(static_cast<float>(m_frame.timeMs));
    satNode->setCameraPosition(m_frame.cameraPos);
    if (m_geometryChanged)
        satNode->setSize(QSizeF(width(), height()));
    if (m_satellitesChanged) {
        satNode->setSatellites(m_satellites);
        m_satellitesChanged = false;
    }

    pinNode->setMVP(m_frame.mvp);
    pinNode->setViewDir(m_frame.viewDir);
    pinNode->setCameraPosition(m_frame.cameraPos);
    pinNode->setHeadScale(static_cast<float>(m_pinHeadSize));
    pinNode->setViewportRect(pixelRect);
    if (m_geometryChanged)
        pinNode->setSize(QSizeF(width(), height()));
    if (m_pinsChanged) {
        pinNode->setPins(m_pins);
        m_pinsChanged = false;
    }

    markerNode->setMVP(m_frame.mvp);
    markerNode->setCameraPosition(m_frame.cameraPos);
    markerNode->setViewDir(m_frame.viewDir);
    markerNode->setSpriteScale(static_cast<float>(m_markerSize));
    markerNode->setViewportRect(pixelRect);
    if (m_geometryChanged)
        markerNode->setSize(QSizeF(width(), height()));
    if (m_markersChanged) {
        markerNode->setMarkers(m_markers);
        m_markersChanged = false;
    }

    smokeNode->setMVP(m_frame.mvp);
    smokeNode->setViewDir(m_frame.viewDir);
    smokeNode->setTime(static_cast<float>(m_frame.timeMs));
    smokeNode->setViewportRect(pixelRect);
    for (const auto &s : m_newSmokeSources)
        smokeNode->addSource(s.lat, s.lon, s.alt);
    m_newSmokeSources.clear();

    introLinesNode->setMVP(m_frame.mvp);
    introLinesNode->setViewDir(m_frame.viewDir);
    introLinesNode->setTime(static_cast<float>(m_frame.timeMs));
    introLinesNode->setDuration(static_cast<float>(m_introDuration));
    introLinesNode->setViewportRect(pixelRect);
    if (m_geometryChanged)
        introLinesNode->setSize(QSizeF(width(), height()));

    if (m_geometryChanged)
        m_geometryChanged = false;

    update(); // Request next frame from SceneGraph
    return root;
}
