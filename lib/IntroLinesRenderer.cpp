#include "IntroLinesRenderer.h"
#include "Utils.h"
#include <QDebug>
#include <QFile>
#include <QRandomGenerator>

IntroLinesRenderer::IntroLinesRenderer() = default;

IntroLinesRenderer::~IntroLinesRenderer() {
    releaseResources();
}

void IntroLinesRenderer::releaseResources() {
    m_rhiResources.releaseResources();
    m_initialized = false;
}

void IntroLinesRenderer::generateLines() {
    m_vertices.clear();
    auto *rng = QRandomGenerator::global();

    int lineCount = 60;
    QColor lineColor("#8FD8D8");
    float introLinesAltitude = 1.10F;

    for (int i = 0; i < lineCount; ++i) {
        float lat = rng->bounded(180.0) - 90.0F;
        float lon = rng->bounded(5.0);
        int segments = 4 + rng->bounded(5);

        if (rng->bounded(1.0) < 0.3) {
            lon = rng->bounded(30.0) - 50.0F;
            segments = 3 + rng->bounded(3);
        }

        QVector3D lastPos;
        for (int j = 0; j < segments; ++j) {
            QVector3D pos =
                Utils::latLonToXYZ(lat, lon - j * 5.0F, Utils::GLOBE_RADIUS * introLinesAltitude);

            Vertex v;
            v.position[0] = pos.x();
            v.position[1] = pos.y();
            v.position[2] = pos.z();
            v.color[0] = lineColor.redF();
            v.color[1] = lineColor.greenF();
            v.color[2] = lineColor.blueF();
            v.longitude = lon - j * 5.0F;

            if (j > 0) {
                // Add line segment (last -> current)
                Vertex vPrev;
                vPrev.position[0] = lastPos.x();
                vPrev.position[1] = lastPos.y();
                vPrev.position[2] = lastPos.z();
                vPrev.color[0] = lineColor.redF();
                vPrev.color[1] = lineColor.greenF();
                vPrev.color[2] = lineColor.blueF();
                vPrev.longitude = lon - (j - 1) * 5.0F;

                m_vertices.push_back(vPrev);
                m_vertices.push_back(v);
            }
            lastPos = pos;
        }
    }
}

void IntroLinesRenderer::render(const RenderState *state) {
    if (m_time > m_duration + 500.0F)
        return;

    QRhiRenderTarget *rt = renderTarget();
    QRhiCommandBuffer *cb = commandBuffer();
    if (!rt || !cb)
        return;

    QRhi *r = rt->rhi();
    if (!m_initialized) {
        initializeRHI(r);
        if (!m_initialized)
            return;
    }

    if (!m_rhiResources.pipeline ||
        m_rhiResources.pipeline->renderPassDescriptor() != rt->renderPassDescriptor()) {
        createPipeline(r);
    }
    if (!m_rhiResources.pipeline)
        return;

    QRhiResourceUpdateBatch *rub = r->nextResourceUpdateBatch();

    if (!m_vertexDataUploaded) {
        rub->uploadStaticBuffer(m_rhiResources.vertexBuffer, m_vertices.data());
        m_vertexDataUploaded = true;
    }

    // Calculate opacity and rotation
    float t = m_time / m_duration;
    float opacity = 0.0F;
    if (t < 0.1F)
        opacity = (t / 0.1F);
    else if (t > 0.8F)
        opacity = std::max(0.0F, (1.0F - t) / 0.2F);
    else
        opacity = 1.0F;

    float rotation = M_PI + (2.0F * M_PI) * t; // Start behind globe, one full revolution

    UniformData u;
    memset(&u, 0, sizeof(UniformData));
    const float *mData = m_mvp.constData();
    for (int i = 0; i < 16; ++i)
        u.mvp[i] = mData[i];

    u.viewDir[0] = m_viewDir.x();
    u.viewDir[1] = m_viewDir.y();
    u.viewDir[2] = m_viewDir.z();

    u.opacity = opacity;
    u.rotation = rotation;
    u.currentTime = m_time;
    u.duration = m_duration;
    rub->updateDynamicBuffer(m_rhiResources.uniformBuffer, 0, sizeof(UniformData), &u);

    cb->resourceUpdate(rub);

    // Set viewport and scissor
    cb->setViewport(QRhiViewport(m_viewportRect.x(), m_viewportRect.y(), m_viewportRect.width(),
                                 m_viewportRect.height()));
    cb->setScissor(QRhiScissor(m_viewportRect.x(), m_viewportRect.y(), m_viewportRect.width(),
                               m_viewportRect.height()));

    cb->setGraphicsPipeline(m_rhiResources.pipeline);
    const QRhiCommandBuffer::VertexInput vb[] = {{m_rhiResources.vertexBuffer, 0}};
    cb->setVertexInput(0, 1, vb);
    cb->setShaderResources(m_rhiResources.shaderBindings);
    cb->draw(m_vertices.size(), 1, 0, 0);
}

void IntroLinesRenderer::initializeRHI(QRhi *rhi) {
    generateLines();
    m_rhiResources.vertexBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                                 m_vertices.size() * sizeof(Vertex));
    m_rhiResources.vertexBuffer->create();

    m_rhiResources.uniformBuffer =
        rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(UniformData));
    m_rhiResources.uniformBuffer->create();

    m_rhiResources.shaderBindings = rhi->newShaderResourceBindings();
    m_rhiResources.shaderBindings->setBindings({QRhiShaderResourceBinding::uniformBuffer(
        0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
        m_rhiResources.uniformBuffer)});
    m_rhiResources.shaderBindings->create();

    createPipeline(rhi);
    m_initialized = true;
}

void IntroLinesRenderer::createPipeline(QRhi *rhi) {
    delete m_rhiResources.pipeline;
    m_rhiResources.pipeline = rhi->newGraphicsPipeline();

    QFile vs(":/shaders/shaders/introlines.vert.qsb");
    QFile fs(":/shaders/shaders/introlines.frag.qsb");
    QShader v, f;
    if (vs.open(QIODevice::ReadOnly)) {
        v = QShader::fromSerialized(vs.readAll());
        vs.close();
    }
    if (fs.open(QIODevice::ReadOnly)) {
        f = QShader::fromSerialized(fs.readAll());
        fs.close();
    }

    m_rhiResources.pipeline->setShaderStages(
        {{QRhiShaderStage::Vertex, v}, {QRhiShaderStage::Fragment, f}});

    QRhiVertexInputLayout layout;
    layout.setBindings({{sizeof(Vertex)}});
    layout.setAttributes({{0, 0, QRhiVertexInputAttribute::Float3, offsetof(Vertex, position)},
                          {0, 1, QRhiVertexInputAttribute::Float3, offsetof(Vertex, color)},
                          {0, 2, QRhiVertexInputAttribute::Float, offsetof(Vertex, longitude)}});
    m_rhiResources.pipeline->setVertexInputLayout(layout);
    m_rhiResources.pipeline->setShaderResourceBindings(m_rhiResources.shaderBindings);

    QRhiRenderTarget *rt = renderTarget();
    if (rt)
        m_rhiResources.pipeline->setRenderPassDescriptor(rt->renderPassDescriptor());

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_rhiResources.pipeline->setTargetBlends({blend});

    m_rhiResources.pipeline->setDepthTest(true);
    m_rhiResources.pipeline->setDepthWrite(false);
    m_rhiResources.pipeline->setTopology(QRhiGraphicsPipeline::Lines);
    m_rhiResources.pipeline->setLineWidth(2.0F);

    m_rhiResources.pipeline->create();
}
