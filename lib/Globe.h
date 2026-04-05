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

class Globe : public QQuickItem {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(qreal dayLength READ dayLength WRITE setDayLength NOTIFY
                 dayLengthChanged) // rotation speed in ms per full turn
  Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY
                 scaleChanged) // zoom factor (typically 0.3 to 3.0)
  Q_PROPERTY(qreal viewAngle READ viewAngle WRITE setViewAngle NOTIFY
                 viewAngleChanged) // vertical tilt in radians
  Q_PROPERTY(QString baseColor READ baseColor WRITE setBaseColor NOTIFY
                 baseColorChanged)
  Q_PROPERTY(
      QString pinColor READ pinColor WRITE setPinColor NOTIFY pinColorChanged)
  Q_PROPERTY(QString markerColor READ markerColor WRITE setMarkerColor NOTIFY
                 markerColorChanged)
  Q_PROPERTY(QString satelliteColor READ satelliteColor WRITE setSatelliteColor
                 NOTIFY satelliteColorChanged)
  Q_PROPERTY(QString introLinesColor READ introLinesColor WRITE
                 setIntroLinesColor NOTIFY introLinesColorChanged)
  Q_PROPERTY(qreal introDuration READ introDuration WRITE setIntroDuration
                 NOTIFY introDurationChanged) // animation duration in ms
  Q_PROPERTY(qreal markerSize READ markerSize WRITE setMarkerSize NOTIFY
                 markerSizeChanged) // scale multiplier for marker sprites
  Q_PROPERTY(
      qreal rotationOffset READ rotationOffset WRITE setRotationOffset NOTIFY
          rotationOffsetChanged) // horizontal rotation offset in radians
  Q_PROPERTY(qreal pinHeadSize READ pinHeadSize WRITE setPinHeadSize NOTIFY
                 pinHeadSizeChanged) // scale multiplier for pin heads
  Q_PROPERTY(bool showLabels READ showLabels WRITE setShowLabels NOTIFY
                 showLabelsChanged)
  Q_PROPERTY(QVariantList pinLabels READ pinLabels NOTIFY pinLabelsChanged)

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

  QVariantList pinLabels() const { return m_pinLabels; }

  QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) override;

  Q_INVOKABLE void addPin(qreal lat, qreal lon,
                          const QString &label = QString());
  Q_INVOKABLE void addMarker(qreal lat, qreal lon, const QString &label,
                             bool connected = false);
  Q_INVOKABLE void
  addSatellite(qreal lat, qreal lon,
               qreal altitude); // altitude as multiplier (e.g. 1.2)
  Q_INVOKABLE void clearSatellites();

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
  void pinLabelsChanged();

protected:
  void geometryChange(const QRectF &newGeometry,
                      const QRectF &oldGeometry) override;
  void componentComplete() override;

private Q_SLOTS:
  void updateState();

private:
  void loadTileData();
  void scheduleUpdate();
  void updateFrameData(); // Centralized calculation

  struct FrameData {
    qint64 timeMs{0};
    float dt{0.0f};
    float cameraAngle{0.0f};
    float cameraDist{0.0f};
    QVector3D cameraPos;
    QVector3D viewDir;
    QMatrix4x4 view;
    QMatrix4x4 projection;
    QMatrix4x4 mvp;
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
  QVariantList m_pinLabels;

  // Markers
  std::vector<MarkerData> m_markers;
  bool m_markersChanged{false};

  // Smoke
  struct NewSmokeSource {
    float lat, lon, alt;
  };
  std::vector<NewSmokeSource> m_newSmokeSources;

  // Timing
  QElapsedTimer m_elapsed;
  qint64 m_startTime{-1};
  qint64 m_lastFrameTime{0};
};
