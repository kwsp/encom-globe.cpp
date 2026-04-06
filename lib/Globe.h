#pragma once

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QMatrix4x4>
#include <QQuickItem>
#include <QTimer>
#include <QVariantList>
#include <QVector3D>
#include <vector>

class GlobeRenderer;
class SatelliteRenderer;
class PinRenderer;
class MarkerRenderer;
class SmokeRenderer;
class IntroLinesRenderer;
struct SatelliteData;
struct PinData;
struct MarkerData;

class GlobeLabel : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

public:
    explicit GlobeLabel(QObject *parent = nullptr) : QObject(parent) {}

    qreal x() const { return m_x; }
    void setX(qreal x) {
        if (!qFuzzyCompare(m_x, x)) {
            m_x = x;
            emit xChanged();
        }
    }

    qreal y() const { return m_y; }
    void setY(qreal y) {
        if (!qFuzzyCompare(m_y, y)) {
            m_y = y;
            emit yChanged();
        }
    }

    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity) {
        if (!qFuzzyCompare(m_opacity, opacity)) {
            m_opacity = opacity;
            emit opacityChanged();
        }
    }

    QString text() const { return m_text; }
    void setText(const QString &text) {
        if (m_text != text) {
            m_text = text;
            emit textChanged();
        }
    }

signals:
    void xChanged();
    void yChanged();
    void opacityChanged();
    void textChanged();

private:
    qreal m_x = 0.0;
    qreal m_y = 0.0;
    qreal m_opacity = 0.0;
    QString m_text;
};

class Globe : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qreal dayLength READ dayLength WRITE setDayLength NOTIFY
                   dayLengthChanged) // rotation speed in ms per full turn
    Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY
                   scaleChanged) // zoom factor (typically 0.3 to 3.0)
    Q_PROPERTY(qreal viewAngle READ viewAngle WRITE setViewAngle NOTIFY
                   viewAngleChanged) // vertical tilt in radians
    Q_PROPERTY(QString baseColor READ baseColor WRITE setBaseColor NOTIFY baseColorChanged)
    Q_PROPERTY(QString pinColor READ pinColor WRITE setPinColor NOTIFY pinColorChanged)
    Q_PROPERTY(QString markerColor READ markerColor WRITE setMarkerColor NOTIFY markerColorChanged)
    Q_PROPERTY(QString satelliteColor READ satelliteColor WRITE setSatelliteColor NOTIFY
                   satelliteColorChanged)
    Q_PROPERTY(QString introLinesColor READ introLinesColor WRITE setIntroLinesColor NOTIFY
                   introLinesColorChanged)
    Q_PROPERTY(qreal introDuration READ introDuration WRITE setIntroDuration NOTIFY
                   introDurationChanged) // animation duration in ms
    Q_PROPERTY(qreal startupDelay READ startupDelay WRITE setStartupDelay NOTIFY
                   startupDelayChanged) // delay before animation starts in ms
    Q_PROPERTY(qreal markerSize READ markerSize WRITE setMarkerSize NOTIFY
                   markerSizeChanged) // scale multiplier for marker sprites
    Q_PROPERTY(qreal rotationOffset READ rotationOffset WRITE setRotationOffset NOTIFY
                   rotationOffsetChanged) // horizontal rotation offset in radians
    Q_PROPERTY(qreal pinHeadSize READ pinHeadSize WRITE setPinHeadSize NOTIFY
                   pinHeadSizeChanged) // scale multiplier for pin heads
    Q_PROPERTY(bool showLabels READ showLabels WRITE setShowLabels NOTIFY showLabelsChanged)
    Q_PROPERTY(QQmlListProperty<QObject> pinLabels READ pinLabels NOTIFY pinLabelsChanged)

public:
    explicit Globe(QQuickItem *parent = nullptr);
    Globe(Globe &&) = delete;
    Globe(const Globe &) = delete;
    Globe operator=(Globe &&) = delete;
    Globe &operator=(const Globe &) = delete;
    ~Globe() override;

    qreal dayLength() const { return m_dayLength; }
    void setDayLength(qreal len);

    qreal scale() const { return m_scale; }
    void setScale(qreal s);

    qreal viewAngle() const { return m_viewAngle; }
    void setViewAngle(qreal angle);

    QString baseColor() const { return m_baseColor; }
    void setBaseColor(const QString &color);

    qreal introDuration() const { return m_introDuration; }
    void setIntroDuration(qreal duration);

    qreal startupDelay() const { return m_startupDelay; }
    void setStartupDelay(qreal delay);

    QString pinColor() const { return m_pinColor; }
    void setPinColor(const QString &color);

    QString markerColor() const { return m_markerColor; }
    void setMarkerColor(const QString &color);

    QString satelliteColor() const { return m_satelliteColor; }
    void setSatelliteColor(const QString &color);

    QString introLinesColor() const { return m_introLinesColor; }
    void setIntroLinesColor(const QString &color);

    qreal markerSize() const { return m_markerSize; }
    void setMarkerSize(qreal size);

    qreal rotationOffset() const { return m_rotationOffset; }
    void setRotationOffset(qreal offset);

    qreal pinHeadSize() const { return m_pinHeadSize; }
    void setPinHeadSize(qreal size);

    bool showLabels() const { return m_showLabels; }
    void setShowLabels(bool show);

    QQmlListProperty<QObject> pinLabels();

    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) override;

    Q_INVOKABLE void addPin(qreal lat, qreal lon, const QString &label = QString());
    Q_INVOKABLE void addMarker(qreal lat, qreal lon, const QString &label, bool connected = false);
    Q_INVOKABLE void addSatellite(qreal lat, qreal lon,
                                  qreal altitude); // altitude as multiplier (e.g. 1.2)
    Q_INVOKABLE void clearSatellites();
    Q_INVOKABLE void clearPins();
    Q_INVOKABLE void clearMarkers();
    Q_INVOKABLE void restartAnimation();

Q_SIGNALS:
    void dayLengthChanged();
    void scaleChanged();
    void viewAngleChanged();
    void baseColorChanged();
    void pinColorChanged();
    void markerColorChanged();
    void satelliteColorChanged();
    void introLinesColorChanged();
    void markerSizeChanged();
    void rotationOffsetChanged();
    void pinHeadSizeChanged();
    void showLabelsChanged();
    void introDurationChanged();
    void startupDelayChanged();
    void pinLabelsChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void componentComplete() override;
    void itemChange(ItemChange change, const ItemChangeData &data) override;

private Q_SLOTS:
    void syncAnimation();

private:
    void updateState();
    void loadTileData();
    void scheduleUpdate();
    void updateFrameData(); // Centralized calculation

    /**
     * @brief Shared state calculated once per frame to be reused by both UI and
     * Render logic.
     */
    struct FrameData {
        float timeMs{};        // Elapsed time since start in milliseconds
        float dt{};            // Delta time (seconds since last frame) for animations
        float cameraAngle{};   // Current orbiting angle in radians
        float cameraDist{};    // Current camera dist from origin
        QVector3D cameraPos;   // Camera pos in world space
        QVector3D viewDir;     // Normalized dir from origin to camera (for depth fog)
        QMatrix4x4 view;       // LookAt view matrix
        QMatrix4x4 projection; // Perspective projection matrix
        QMatrix4x4 mvp;        // Combined Model-View-Projection matrix
    } m_frame;

    // Configuration
    qreal m_dayLength{28000.0};
    qreal m_scale{1.0};
    qreal m_viewAngle{0.0};
    QString m_baseColor{"#ffcc00"};
    QString m_pinColor{"#00eeee"};
    QString m_markerColor{"#ffcc00"};
    QString m_satelliteColor{"#ff0000"};
    QString m_introLinesColor{"#8FD8D8"};
    qreal m_markerSize{1.0};
    qreal m_rotationOffset{0.0};
    qreal m_pinHeadSize{0.2};
    bool m_showLabels{false};
    qreal m_introDuration{2000.0};
    qreal m_startupDelay{0.0};

    // State
    QJsonObject m_tileData;
    bool m_tilesLoaded = false;
    bool m_geometryChanged = false;

    // Satellites
    std::vector<SatelliteData> m_satellites;
    bool m_satellitesChanged{false};

    // Pins
    std::vector<PinData> m_pins;
    bool m_pinsChanged{false};
    QList<QObject *> m_pinLabelsList;

    // Markers
    std::vector<MarkerData> m_markers;
    bool m_markersChanged{false};
    bool m_clearSmoke{false};

    // Smoke
    struct NewSmokeSource {
        float lat, lon, alt;
    };
    std::vector<NewSmokeSource> m_newSmokeSources;

    // Timing
    QElapsedTimer m_elapsed;
    qint64 m_startTime{-1};
    qint64 m_lastFrameTime{0};
    float m_lastActiveTimeMs{0.0f};
};
