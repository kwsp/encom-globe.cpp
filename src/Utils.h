#pragma once

#include <QVector3D>
#include <QColor>
#include <QRandomGenerator>
#include <cmath>

namespace Utils {

constexpr float GLOBE_RADIUS = 500.0F;
constexpr float CAMERA_DISTANCE = 1700.0F;

inline QVector3D latLonToXYZ(float lat, float lon, float scale = GLOBE_RADIUS) {
    const float phi = qDegreesToRadians(90.0F - lat);
    const float theta = qDegreesToRadians(180.0F - lon);
    return {
        scale * std::sin(phi) * std::cos(theta),
        scale * std::cos(phi),
        scale * std::sin(phi) * std::sin(theta)
    };
}

inline QVector3D latLonToXYZ(double lat, double lon, double scale = GLOBE_RADIUS) {
    return latLonToXYZ(static_cast<float>(lat), static_cast<float>(lon), static_cast<float>(scale));
}

inline QColor hexToColor(const QString& hex) {
    QString h = hex;
    if (h.startsWith('#')) {
        h = h.mid(1);
    }
    if (h.length() == 3) {
        h = QString("%1%1%2%2%3%3").arg(h[0]).arg(h[1]).arg(h[2]);
    }
    bool ok = false;
    const int rgb = h.toInt(&ok, 16);
    if (ok) {
        return QColor((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
    }
    return QColor(0xFF, 0xCC, 0x00); // default golden
}

inline QColor randomColorVariation(const QColor& base, float variance = 0.2f) {
    float h, s, v, a;
    base.getHsvF(&h, &s, &v, &a);
    
    auto* rng = QRandomGenerator::global();
    
    // Random hue shift
    h += (rng->bounded(1.0) - 0.5f) * variance * 0.1f;
    while (h < 0) h += 1.0f;
    while (h > 1) h -= 1.0f;
    
    // Random value shift
    v += (rng->bounded(1.0) - 0.5f) * variance;
    v = qBound(0.0f, v, 1.0f);
    
    QColor result;
    result.setHsvF(h, s, v, a);
    return result;
}

} // namespace Utils
