#pragma once

#include <QColor>
#include <QMatrix4x4>
#include <QSGRenderNode>
#include <rhi/qrhi.h>
#include <vector>

struct MarkerData {
    float lat;
    float lon;
    float altitude = 1.2F;
    QString text;
    QColor color = QColor("#FFCC00");
    int previousIndex = -1;      // Index of connected marker, -1 if none
    float lineProgress = 0.0F;   // 0-1 for arc draw animation
    float markerProgress = 0.0F; // 0-1 for marker sprite pop-in
    QVector3D basePos;
    QVector3D pos;
};

class MarkerRenderer : public QSGRenderNode {
public:
    MarkerRenderer();
    MarkerRenderer(MarkerRenderer &&) = delete;
    MarkerRenderer(const MarkerRenderer &) = delete;
    MarkerRenderer &operator=(MarkerRenderer &&) = delete;
    MarkerRenderer &operator=(const MarkerRenderer &) = delete;
    ~MarkerRenderer() override;

    StateFlags changedStates() const override { return BlendState; }
    RenderingFlags flags() const override { return BoundedRectRendering | DepthAwareRendering; }
    void prepare() override {}
    void render(const RenderState *state) override;
    void releaseResources() override;
    QRectF rect() const override { return {0, 0, m_size.width(), m_size.height()}; }

    void setMVP(const QMatrix4x4 &mvp) { m_mvp = mvp; }
    void setCameraPosition(const QVector3D &pos) { m_cameraPos = pos; }
    void setViewDir(const QVector3D &dir) { m_viewDir = dir; }
    void setSize(const QSizeF &size) { m_size = size; }
    void setViewportRect(const QRect &rect) { m_viewportRect = rect; }
    void setSpriteScale(float scale) { m_spriteScale = scale; }
    void setMarkers(const std::vector<MarkerData> &markers);

private:
    static constexpr int LINE_SEGMENTS = 100;
    static constexpr int MAX_MARKERS = 64;
    static constexpr int UNIFORM_ALIGNMENT = 256;

    void initializeRHI(QRhi *rhi);
    void createLinePipeline(QRhi *rhi);
    void createSpritePipeline(QRhi *rhi);
    void rebuildArcGeometry();

    // Line resources
    QRhiGraphicsPipeline *m_linePipeline = nullptr;
    QRhiBuffer *m_lineVertexBuffer = nullptr;
    QRhiBuffer *m_lineUniformBuffer = nullptr;
    QRhiShaderResourceBindings *m_lineBindings = nullptr;

    // Marker sprite resources
    QRhiGraphicsPipeline *m_spritePipeline = nullptr;
    QRhiBuffer *m_spriteVertexBuffer = nullptr;
    QRhiBuffer *m_spriteUniformBuffer = nullptr;
    QRhiShaderResourceBindings *m_spriteBindings = nullptr;
    bool m_spriteVertexUploaded = false;

    bool m_initialized = false;
    bool m_needsPipelineRebuild = false;
    bool m_geometryDirty = false;

    struct LineVertex {
        float position[3];
        float color[3];
    };

    struct LineUniformData {
        float mvp[16];
        float viewDir[3];
        float padding;
    }; // 80 bytes

    struct SpriteUniformData {
        float mvp[16];       // offset 0
        float color[3];      // offset 64
        float relativeDepth; // offset 76
    }; // 80 bytes

    // Per-connection arc data
    struct ArcConnection {
        int fromIdx;
        int toIdx;
        int vertexOffset;  // Offset into vertex buffer
        int totalVertices; // Total vertices for this arc (LINE_SEGMENTS+1)
    };

    std::vector<MarkerData> m_markers;
    std::vector<LineVertex> m_arcVertices;
    std::vector<ArcConnection> m_arcs;
    std::vector<SpriteUniformData> m_spriteUniformCache;
    QMatrix4x4 m_mvp;
    QVector3D m_cameraPos;
    QVector3D m_viewDir;
    float m_spriteScale = 1.0F;
    QRect m_viewportRect;
    QSizeF m_size;
    int m_lineVertexCapacity = 0;
};
