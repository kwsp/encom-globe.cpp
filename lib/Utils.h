#pragma once

#include <QColor>
#include <QRandomGenerator>
#include <QVector3D>
#include <cmath>
#include <numbers>

namespace Utils {

constexpr float GLOBE_RADIUS = 500.0F;
constexpr float CAMERA_DISTANCE = 1700.0F;

inline QVector3D latLonToXYZ(float lat, float lon, float scale = GLOBE_RADIUS) {
    const float phi = qDegreesToRadians(90.0F - lat);
    const float theta = qDegreesToRadians(180.0F - lon);
    return {scale * std::sin(phi) * std::cos(theta), scale * std::cos(phi),
            scale * std::sin(phi) * std::sin(theta)};
}

inline QVector3D latLonToXYZ(double lat, double lon, double scale = GLOBE_RADIUS) {
    return latLonToXYZ(static_cast<float>(lat), static_cast<float>(lon), static_cast<float>(scale));
}

inline QColor hexToColor(const QString &hex) {
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
        return {(rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF};
    }
    return {0xFF, 0xCC, 0x00}; // default golden
}

// Generate a palette matching pusherColor().hueSet()
// Same hue, varying saturation (100,65,30) and value (100,65,30) = 9 colors
inline std::vector<QColor> hueSet(const QColor &base) {
    float h, s, v, a;
    base.getHsvF(&h, &s, &v, &a);

    std::vector<QColor> colors;
    // saturation: 100%, 65%, 30%  value: 100%, 65%, 30%
    float sats[] = {1.0F, 0.65F, 0.30F};
    float vals[] = {1.0F, 0.65F, 0.30F};
    for (float sv : sats) {
        for (float vv : vals) {
            QColor c;
            c.setHsvF(h, sv, vv, 1.0F);
            colors.push_back(c);
        }
    }
    return colors;
}

// Pick a random color from the hue set and blend toward black by 0-33%
// (matches shade(Math.random()/3.0) in the original)
inline QColor randomTileColor(const std::vector<QColor> &palette) {
    auto *rng = QRandomGenerator::global();
    int idx = rng->bounded(static_cast<int>(palette.size()));
    QColor c = palette[idx];

    // shade(amount) = blend toward black by amount
    float amount = rng->bounded(1.0) / 3.0F;
    int r = static_cast<int>(c.red() * (1.0F - amount));
    int g = static_cast<int>(c.green() * (1.0F - amount));
    int b = static_cast<int>(c.blue() * (1.0F - amount));
    return QColor(r, g, b);
}

inline QColor randomColorVariation(const QColor &base, float variance = 0.2F) {
    float h, s, v, a;
    base.getHsvF(&h, &s, &v, &a);

    auto *rng = QRandomGenerator::global();

    h += (rng->bounded(1.0) - 0.5F) * variance * 0.1F;
    while (h < 0)
        h += 1.0F;
    while (h > 1)
        h -= 1.0F;

    v += (rng->bounded(1.0) - 0.5F) * variance;
    v = qBound(0.0F, v, 1.0F);

    QColor result;
    result.setHsvF(h, s, v, a);
    return result;
}

inline float elasticOut(float t) {
    if (t <= 0.0F)
        return 0.0F;
    if (t >= 1.0F)
        return 1.0F;
    float p = 0.3F;
    return std::pow(2.0F, -10.0F * t) *
               std::sin((t - p / 4.0F) * (2.0F * std::numbers::pi_v<float>) / p) +
           1.0F;
}

} // namespace Utils
