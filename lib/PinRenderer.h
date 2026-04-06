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
    QVector3D basePos;
    QVector3D endPos;
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
    void setCameraPosition(const QVector3D& pos) { m_cameraPos = pos; }
    void setViewDir(const QVector3D& dir) { m_viewDir = dir; }
    void setHeadScale(float scale) { m_headScale = scale; }
    void setViewportRect(const QRect& rect) { m_viewportRect = rect; }
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

    // Top sprite resources
    QRhiGraphicsPipeline* m_topPipeline = nullptr;
    QRhiBuffer* m_topVertexBuffer = nullptr;
    QRhiBuffer* m_topUniformBuffer = nullptr;
    QRhiShaderResourceBindings* m_topShaderBindings = nullptr;

    bool m_initialized = false;
    bool m_needsPipelineRebuild = false;
    bool m_geometryDirty = false;
    bool m_topVertexDataUploaded = false;

    struct Vertex {
        float position[3];
        float color[3];
    };

    struct TopVertex {
        float offset[2];
    };

    struct UniformData {
        float mvp[16];
        float viewDir[3];
        float padding;
    }; // 80 bytes

    struct TopUniformData {
        float mvp[16];        // offset 0
        float color[3];       // offset 64
        float relativeDepth;  // offset 76
        float viewDir[3];     // offset 80
        float padding2;       // offset 92
    }; // Total: 96 bytes, std140 aligned

    std::vector<PinData> m_pins;
    std::vector<TopUniformData> m_topUniformDataCache;
    QMatrix4x4 m_mvp;
    QVector3D m_cameraPos;
    QVector3D m_viewDir;
    float m_headScale = 1.0f;
    QRect m_viewportRect;
    QSizeF m_size;
    int m_vertexCapacity = 0;
};
