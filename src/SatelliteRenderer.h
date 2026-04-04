#pragma once

#include <QSGRenderNode>
#include <rhi/qrhi.h>
#include <QMatrix4x4>
#include <QColor>
#include <vector>

struct SatelliteData {
    float lat;
    float lon;
    float altitude;
    QColor waveColor = "#FFFFFF";
    QColor coreColor = "#FF0000";
    QColor shieldColor = "#FFFFFF";
    float size = 1.0f;
};

class SatelliteRenderer : public QSGRenderNode {
public:
    SatelliteRenderer();
    ~SatelliteRenderer() override;

    // QSGRenderNode interface
    StateFlags changedStates() const override { return BlendState; }
    RenderingFlags flags() const override { return BoundedRectRendering; }
    void prepare() override {}
    void render(const RenderState* state) override;
    void releaseResources() override;
    QRectF rect() const override { return {0, 0, m_size.width(), m_size.height()}; }

    // Data setters
    void setMVP(const QMatrix4x4& mvp) { m_mvp = mvp; }
    void setView(const QMatrix4x4& view) { m_view = view; }
    void setTime(float time) { m_time = time; }
    void setCameraPosition(const QVector3D& pos) { m_cameraPos = pos; }
    void setCameraAngle(float angle) { m_cameraAngle = angle; }
    void setSize(const QSizeF& size) { m_size = size; }
    
    // Satellite management
    void setSatellites(const std::vector<SatelliteData>& satellites);
    void clearSatellites();

private:
    void initializeRHI(QRhi* rhi);
    void createPipeline(QRhi* rhi);

    // RHI resources
    QRhiGraphicsPipeline* m_pipeline = nullptr;
    QRhiBuffer* m_vertexBuffer = nullptr;
    QRhiBuffer* m_uniformBuffer = nullptr;
    QRhiShaderResourceBindings* m_shaderBindings = nullptr;
    
    // Initialization state
    bool m_initialized = false;
    bool m_needsPipelineRebuild = false;
    bool m_vertexDataUploaded = false;
    
    // Vertex data for a single billboard quad (6 vertices for 2 triangles)
    struct Vertex {
        float offset[2];    // Billboard offset (-0.5 to 0.5)
    };
    
    // Uniform data per satellite (std140 aligned)
    struct UniformData {
        float mvp[16];        // offset 0, 64 bytes
        float position[3];    // offset 64, 12 bytes
        float time;           // offset 76, 4 bytes
        float size;           // offset 80, 4 bytes
        float _pad0;          // offset 84, 4 bytes
        float _pad1;          // offset 88, 4 bytes
        float _pad2;          // offset 92, 4 bytes
        float waveColor[3];   // offset 96, 12 bytes
        float arcAngle;       // offset 108, 4 bytes
    };

    // Data
    std::vector<SatelliteData> m_satellites;
    std::vector<UniformData> m_uniformDataCache;  // Cache for uniform data
    QMatrix4x4 m_mvp;
    QMatrix4x4 m_view;
    QVector3D m_cameraPos;
    float m_cameraAngle = 0.0f;
    float m_time = 0.0f;
    QSizeF m_size;
    
    // Track if satellites changed
    bool m_satellitesChanged = false;
};
