#pragma once

#include <QColor>
#include <QMatrix4x4>
#include <QSGRenderNode>
#include <rhi/qrhi.h>
#include <vector>
#include "RhiResources.h"

struct SmokeSource {
    float lat;
    float lon;
    float altitude;
    bool active;
    int firstParticleIdx;
};

class SmokeRenderer : public QSGRenderNode {
public:
    SmokeRenderer();
    SmokeRenderer(SmokeRenderer &&) = delete;
    SmokeRenderer(const SmokeRenderer &) = delete;
    SmokeRenderer operator=(SmokeRenderer &&) = delete;
    SmokeRenderer &operator=(const SmokeRenderer &) = delete;
    ~SmokeRenderer() override;

    StateFlags changedStates() const override { return BlendState; }
    RenderingFlags flags() const override { return BoundedRectRendering | DepthAwareRendering; }
    void prepare() override {}
    void render(const RenderState *state) override;
    void releaseResources() override;
    QRectF rect() const override { return {0, 0, m_size.width(), m_size.height()}; }

    void setMVP(const QMatrix4x4 &mvp) { m_mvp = mvp; }
    void setViewDir(const QVector3D &dir) { m_viewDir = dir; }
    void setTime(float time) { m_time = time; }
    void setViewportRect(const QRect &rect) { m_viewportRect = rect; }
    void setSize(const QSizeF &size) { m_size = size; }

    void addSource(float lat, float lon, float altitude);

private:
    static constexpr int MAX_PARTICLES = 5000;
    static constexpr int PARTICLES_PER_SOURCE = 30;

    void initializeRHI(QRhi *rhi);
    void createPipeline(QRhi *rhi);

    RhiResources m_rhiResources;

    bool m_initialized{};
    bool m_geometryDirty{};

    struct ParticleVertex {
        float startLat;
        float startLon;
        float altitude;
        float startTime;
        float isActive; // 1.0 if active, 0.0 otherwise
    };

    struct UniformData {
        float mvp[16];     // 64 bytes
        float viewDir[3];  // 12 bytes (offset 64)
        float currentTime; // 4 bytes (offset 76)
        float color[4];    // 16 bytes (offset 80)
    }; // Total: 96 bytes, 16-byte aligned

    std::vector<ParticleVertex> m_particles;
    int m_nextParticleIdx = 0;
    QMatrix4x4 m_mvp;
    QVector3D m_viewDir;
    float m_time{0.0F};
    QRect m_viewportRect;
    QSizeF m_size;
};
