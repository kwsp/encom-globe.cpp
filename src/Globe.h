#pragma once

#include <QQuickItem>
#include <QElapsedTimer>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantList>
#include <vector>

class GlobeRenderer;
class SatelliteRenderer;
class PinRenderer;
class MarkerRenderer;
struct SatelliteData;
struct PinData;
struct MarkerData;

class Globe : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qreal dayLength READ dayLength WRITE setDayLength NOTIFY dayLengthChanged)
    Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY scaleChanged)
    Q_PROPERTY(qreal viewAngle READ viewAngle WRITE setViewAngle NOTIFY viewAngleChanged)
    Q_PROPERTY(QString baseColor READ baseColor WRITE setBaseColor NOTIFY baseColorChanged)
    Q_PROPERTY(qreal introDuration READ introDuration WRITE setIntroDuration NOTIFY introDurationChanged)
    Q_PROPERTY(QVariantList pinLabels READ pinLabels NOTIFY pinLabelsChanged)

public:
    explicit Globe(QQuickItem* parent = nullptr);
    ~Globe() override;

    qreal dayLength() const { return m_dayLength; }
    void setDayLength(qreal len);

    qreal scale() const { return m_scale; }
    void setScale(qreal s);

    qreal viewAngle() const { return m_viewAngle; }
    void setViewAngle(qreal angle);

    QString baseColor() const { return m_baseColor; }
    void setBaseColor(const QString& color);

    qreal introDuration() const { return m_introDuration; }
    void setIntroDuration(qreal duration);

    QVariantList pinLabels() const { return m_pinLabels; }

    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData*) override;

    Q_INVOKABLE void addPin(qreal lat, qreal lon, const QString& label = QString());
    Q_INVOKABLE void addMarker(qreal lat, qreal lon, const QString& label, bool connected = false);
    Q_INVOKABLE void addSatellite(qreal lat, qreal lon, qreal altitude);
    Q_INVOKABLE void clearSatellites();

Q_SIGNALS:
    void dayLengthChanged();
    void scaleChanged();
    void viewAngleChanged();
    void baseColorChanged();
    void introDurationChanged();
    void pinLabelsChanged();

protected:
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void componentComplete() override;

private Q_SLOTS:
    void updateState();

private:
    void loadTileData();
    void scheduleUpdate();

    // Configuration
    qreal m_dayLength = 28000.0;
    qreal m_scale = 1.0;
    qreal m_viewAngle = 0.0;
    QString m_baseColor = "#ffcc00";
    qreal m_introDuration = 2000.0;

    // State
    QJsonObject m_tileData;
    bool m_tilesLoaded = false;
    bool m_geometryChanged = false;
    
    // Satellites
    std::vector<SatelliteData> m_satellites;
    bool m_satellitesChanged = false;
    
    // Pins
    std::vector<PinData> m_pins;
    bool m_pinsChanged = false;
    QVariantList m_pinLabels;
    
    // Markers
    std::vector<MarkerData> m_markers;
    bool m_markersChanged = false;
    
    // Timing
    QElapsedTimer m_elapsed;
    qint64 m_startTime = 0;
    QTimer* m_updateTimer = nullptr;
};
