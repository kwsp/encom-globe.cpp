#pragma once

#include <QSGRenderNode>
#include <QSGRendererInterface>
#include <rhi/qrhi.h>
#include <QMatrix4x4>
#include <QColor>
#include <QJsonObject>
#include <QSizeF>
#include <vector>
#include <memory>

class GlobeRenderer : public QSGRenderNode {
public:
    GlobeRenderer();
    ~GlobeRenderer() override;

    // QSGRenderNode interface
    StateFlags changedStates() const override { return DepthState; }
    RenderingFlags flags() const override { return DepthAwareRendering; }
    void prepare() override {}
    void render(const RenderState* state) override;
    void releaseResources() override;
    QRectF rect() const override { return QRectF(0, 0, m_size.width(), m_size.height()); }

    // Data setters (called from Globe)
    void setMVP(const QMatrix4x4& mvp) { m_mvp = mvp; }
    void setCurrentTime(float time) { m_currentTime = time; }
    void setIntroDuration(float duration) { m_introDuration = duration; }
    void setIntroAltitude(float altitude) { m_introAltitude = altitude; }
    void setBaseColor(const QString& color);
    void setTileData(const QJsonObject& data);
    void setSize(const QSizeF& size) { m_size = size; }

    // Tick for animation
    void tick();

private:
    void initializeRHI(QRhi* rhi);
    void createPipeline(QRhi* rhi);
    void createBuffers(QRhi* rhi);
    void updateUniformBuffer(QRhiResourceUpdateBatch* batch);

    // RHI resources
    QRhiGraphicsPipeline* m_pipeline = nullptr;
    QRhiBuffer* m_vertexBuffer = nullptr;
    QRhiBuffer* m_uniformBuffer = nullptr;
    QRhiShaderResourceBindings* m_shaderBindings = nullptr;
    
    // Initialization state
    bool m_initialized = false;
    bool m_needsPipelineRebuild = false;
    
    // Vertex data
    struct Vertex {
        float position[3];
        float color[3];
        float longitude;
    };
    std::vector<Vertex> m_vertices;
    bool m_vertexDataChanged = false;

    // Uniform data
    QMatrix4x4 m_mvp;
    float m_currentTime = 0.0f;
    float m_introDuration = 2000.0f;
    float m_introAltitude = 1.10f;
    QString m_baseColor = "#ffcc00";
    QJsonObject m_tileData;
    
    // Size
    QSizeF m_size;
};
