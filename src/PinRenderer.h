#pragma once

#include <QSGRenderNode>
#include <rhi/qrhi.h>
#include <QMatrix4x4>
#include <QColor>
#include <vector>

struct PinData {
    float lat;
    float lon;
    float altitude;
    QColor color = QColor("#8FD8D8");
    QString text;
    float progress = 1.0f; // 0.0 to 1.0 for elastic growth animation
};

class PinRenderer : public QSGRenderNode {
public:
    PinRenderer();
    ~PinRenderer() override;

    StateFlags changedStates() const override { return BlendState; }
    RenderingFlags flags() const override { return BoundedRectRendering | DepthAwareRendering; }
    void prepare() override {}
    void render(const RenderState* state) override;
    void releaseResources() override;
    QRectF rect() const override { return {0, 0, m_size.width(), m_size.height()}; }

    void setMVP(const QMatrix4x4& mvp) { m_mvp = mvp; }
    void setSize(const QSizeF& size) { m_size = size; }
    void setPins(const std::vector<PinData>& pins);

private:
    void initializeRHI(QRhi* rhi);
    void createPipeline(QRhi* rhi);
    void updateBuffers(QRhi* rhi, QRhiResourceUpdateBatch* rub);

    QRhiGraphicsPipeline* m_pipeline = nullptr;
    QRhiBuffer* m_vertexBuffer = nullptr;
    QRhiBuffer* m_uniformBuffer = nullptr;
    QRhiShaderResourceBindings* m_shaderBindings = nullptr;

    bool m_initialized = false;
    bool m_needsPipelineRebuild = false;
    bool m_geometryDirty = false;

    struct Vertex {
        float position[3];
        float color[3];
    };

    struct UniformData {
        float mvp[16];
    };

    std::vector<PinData> m_pins;
    QMatrix4x4 m_mvp;
    QSizeF m_size;
    int m_vertexCapacity = 0;
};
