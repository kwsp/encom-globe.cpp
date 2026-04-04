#pragma once

#include <QSGRenderNode>
#include <rhi/qrhi.h>
#include <QMatrix4x4>
#include <QColor>
#include <vector>

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
    ~SmokeRenderer() override;

    StateFlags changedStates() const override { return BlendState; }
    RenderingFlags flags() const override { return BoundedRectRendering | DepthAwareRendering; }
    void prepare() override {}
    void render(const RenderState* state) override;
    void releaseResources() override;
    QRectF rect() const override { return {0, 0, m_size.width(), m_size.height()}; }

    void setMVP(const QMatrix4x4& mvp) { m_mvp = mvp; }
    void setTime(float time) { m_time = time; }
    void setCameraDistance(float dist) { m_cameraDistance = dist; }
    void setSize(const QSizeF& size) { m_size = size; }
    
    void addSource(float lat, float lon, float altitude);

private:
    static constexpr int MAX_PARTICLES = 5000;
    static constexpr int PARTICLES_PER_SOURCE = 30;

    void initializeRHI(QRhi* rhi);
    void createPipeline(QRhi* rhi);

    QRhiGraphicsPipeline* m_pipeline = nullptr;
    QRhiBuffer* m_vertexBuffer = nullptr;
    QRhiBuffer* m_uniformBuffer = nullptr;
    QRhiShaderResourceBindings* m_bindings = nullptr;

    bool m_initialized = false;
    bool m_geometryDirty = false;

    struct ParticleVertex {
        float startLat;
        float startLon;
        float altitude;
        float startTime;
        float isActive; // 1.0 if active, 0.0 otherwise
    };

    struct UniformData {
        float mvp[16];       // 64 bytes
        float color[4];      // 16 bytes
        float currentTime;   // 4 bytes
        float cameraDistance;// 4 bytes
        float padding[2];    // 8 bytes
    }; // Total: 96 bytes, 16-byte aligned

    std::vector<ParticleVertex> m_particles;
    int m_nextParticleIdx = 0;
    QMatrix4x4 m_mvp;
    float m_time = 0.0f;
    float m_cameraDistance = 1700.0f;
    QSizeF m_size;
};
