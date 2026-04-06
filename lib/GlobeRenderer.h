#pragma once

#include <QColor>
#include <QJsonObject>
#include <QMatrix4x4>
#include <QSGRenderNode>
#include <QSGRendererInterface>
#include <QSizeF>
#include <rhi/qrhi.h>
#include <vector>

class GlobeRenderer : public QSGRenderNode {
public:
    GlobeRenderer();
    GlobeRenderer(GlobeRenderer &&) = delete;
    GlobeRenderer(const GlobeRenderer &) = delete;
    GlobeRenderer operator=(GlobeRenderer &&) = delete;
    GlobeRenderer &operator=(const GlobeRenderer &) = delete;
    ~GlobeRenderer() override;

    // QSGRenderNode interface
    StateFlags changedStates() const override { return DepthState; }
    RenderingFlags flags() const override { return DepthAwareRendering; }
    void prepare() override {}
    void render(const RenderState *state) override;
    void releaseResources() override;
    QRectF rect() const override { return QRectF(0, 0, m_size.width(), m_size.height()); }

    // Data setters (called from Globe)
    void setMVP(const QMatrix4x4 &mvp) { m_mvp = mvp; }
    void setViewDir(const QVector3D &dir) { m_viewDir = dir; }
    void setCurrentTime(float time) { m_currentTime = time; }
    void setIntroDuration(float duration) { m_introDuration = duration; }
    void setIntroAltitude(float altitude) { m_introAltitude = altitude; }
    void setBaseColor(const QString &color);
    void setTileData(const QJsonObject &data);
    void setViewportRect(const QRect &rect) { m_viewportRect = rect; }
    void setSize(const QSizeF &size) { m_size = size; }

    // Tick for animation
    void tick();

private:
    void initializeRHI(QRhi *rhi);
    void createPipeline(QRhi *rhi);
    void createBuffers(QRhi *rhi);
    void updateUniformBuffer(QRhiResourceUpdateBatch *batch);

    // RHI resources
    QRhiGraphicsPipeline *m_pipeline{};
    QRhiBuffer *m_vertexBuffer{};
    QRhiBuffer *m_uniformBuffer{};
    QRhiShaderResourceBindings *m_shaderBindings{};

    // Initialization state
    bool m_initialized{};
    bool m_needsPipelineRebuild{};

    // Vertex data
    struct Vertex {
        float position[3];
        float color[3];
        float longitude;
    };
    std::vector<Vertex> m_vertices;
    bool m_vertexDataChanged{};

    // Uniform data
    struct UniformData {
        float mvp[16];       // 64 bytes
        float viewDir[3];    // 12 bytes (offset 64)
        float currentTime;   // 4 bytes (offset 76)
        float introDuration; // 4 bytes (offset 80)
        float introAltitude; // 4 bytes (offset 84)
        float padding[2];    // 8 bytes (offset 88)
    }; // Total: 96 bytes, 16-byte aligned

    QMatrix4x4 m_mvp;
    QVector3D m_viewDir;
    float m_currentTime{};
    float m_introDuration{2000.0F};
    float m_introAltitude{1.10F};
    QString m_baseColor{"#ffcc00"};
    QJsonObject m_tileData;

    // Size
    QSizeF m_size;
    QRect m_viewportRect;
};
