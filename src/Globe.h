#pragma once

#include <QQuickItem>
#include <QTimer>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>

class GlobeRenderer;
class SatelliteRenderer;
class PinRenderer;
struct SatelliteData;
struct PinData;

class Globe : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qreal dayLength READ dayLength WRITE setDayLength NOTIFY dayLengthChanged)
    Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY scaleChanged)
    Q_PROPERTY(qreal viewAngle READ viewAngle WRITE setViewAngle NOTIFY viewAngleChanged)
    Q_PROPERTY(QString baseColor READ baseColor WRITE setBaseColor NOTIFY baseColorChanged)
    Q_PROPERTY(qreal introDuration READ introDuration WRITE setIntroDuration NOTIFY introDurationChanged)

public:
    explicit Globe(QQuickItem* parent = nullptr);
    ~Globe() override;

    // Property accessors
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

    // QQuickItem interface
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData*) override;

    // API for adding elements (to be expanded)
    Q_INVOKABLE void addPin(qreal lat, qreal lon, const QString& label = QString());
    Q_INVOKABLE void addMarker(qreal lat, qreal lon, const QString& label, bool connected = false);
    Q_INVOKABLE void addSatellite(qreal lat, qreal lon, qreal altitude);
    Q_INVOKABLE void clearSatellites();

signals:
    void dayLengthChanged();
    void scaleChanged();
    void viewAngleChanged();
    void baseColorChanged();
    void introDurationChanged();

protected:
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void componentComplete() override;

private:
    void loadTileData();
    void scheduleUpdate();

    // Configuration
    qreal m_dayLength = 28000.0;  // ms per rotation
    qreal m_scale = 1.0;
    qreal m_viewAngle = 0.0;
    QString m_baseColor = "#ffcc00";
    qreal m_introDuration = 2000.0;  // ms

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
    
    // Timing
    QElapsedTimer m_elapsed;
    qint64 m_startTime = 0;
};
