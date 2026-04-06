#pragma once

#include <QColor>
#include <QMatrix4x4>
#include <QSGRenderNode>
#include <rhi/qrhi.h>
#include <vector>

class IntroLinesRenderer : public QSGRenderNode {
public:
    IntroLinesRenderer();
    IntroLinesRenderer(IntroLinesRenderer &&) = delete;
    IntroLinesRenderer(const IntroLinesRenderer &) = delete;
    IntroLinesRenderer &operator=(IntroLinesRenderer &&) = delete;
    IntroLinesRenderer &operator=(const IntroLinesRenderer &) = delete;
    ~IntroLinesRenderer() override;

    StateFlags changedStates() const override { return BlendState; }
    RenderingFlags flags() const override { return BoundedRectRendering | DepthAwareRendering; }
    void prepare() override {}
    void render(const RenderState *state) override;
    void releaseResources() override;
    QRectF rect() const override { return {0, 0, m_size.width(), m_size.height()}; }

    void setMVP(const QMatrix4x4 &mvp) { m_mvp = mvp; }
    void setTime(float time) { m_time = time; }
    void setDuration(float duration) { m_duration = duration; }
    void setViewDir(const QVector3D &dir) { m_viewDir = dir; }
    void setViewportRect(const QRect &rect) { m_viewportRect = rect; }
    void setSize(const QSizeF &size) { m_size = size; }

private:
    void initializeRHI(QRhi *rhi);
    void createPipeline(QRhi *rhi);
    void generateLines();

    QRhiGraphicsPipeline *m_pipeline{};
    QRhiBuffer *m_vertexBuffer{};
    QRhiBuffer *m_uniformBuffer{};
    QRhiShaderResourceBindings *m_bindings{};

    bool m_initialized{};
    bool m_vertexDataUploaded{};

    struct Vertex {
        float position[3];
        float color[3];
        float longitude;
    };

    struct UniformData {
        float mvp[16];
        float viewDir[3];
        float opacity;
        float rotation;
        float currentTime;
        float duration;
        float padding;
    }; // Total: 96 bytes

    std::vector<Vertex> m_vertices;
    QMatrix4x4 m_mvp;
    QVector3D m_viewDir;
    float m_time = 0.0F;
    float m_duration = 2000.0F;
    QRect m_viewportRect;
    QSizeF m_size;
};
