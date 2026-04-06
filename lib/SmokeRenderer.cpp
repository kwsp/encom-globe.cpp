#include "SmokeRenderer.h"
#include <QDebug>
#include <QFile>
#include <QRandomGenerator>

SmokeRenderer::SmokeRenderer() {
    m_particles.resize(MAX_PARTICLES, {0, 0, 0, 0, 0});
}

SmokeRenderer::~SmokeRenderer() {
    releaseResources();
}

void SmokeRenderer::releaseResources() {
    m_rhiResources.releaseResources();
    m_initialized = false;
}

void SmokeRenderer::addSource(float lat, float lon, float altitude) {
    auto *rng = QRandomGenerator::global();
    for (int i = 0; i < PARTICLES_PER_SOURCE; ++i) {
        int idx = m_nextParticleIdx;
        m_particles[idx].startLat = lat;
        m_particles[idx].startLon = lon;
        m_particles[idx].altitude = altitude;
        // Stagger start times: current time + some spread
        m_particles[idx].startTime = m_time + (1500.0F * i / PARTICLES_PER_SOURCE);
        m_particles[idx].isActive = 1.0F;

        m_nextParticleIdx = (m_nextParticleIdx + 1) % MAX_PARTICLES;
    }
    m_geometryDirty = true;
}

void SmokeRenderer::render(const RenderState *state) {
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

    // Update uniform buffer
    UniformData u;
    memset(&u, 0, sizeof(UniformData));
    const float *mData = m_mvp.constData();
    for (int i = 0; i < 16; ++i)
        u.mvp[i] = mData[i];

    u.viewDir[0] = m_viewDir.x();
    u.viewDir[1] = m_viewDir.y();
    u.viewDir[2] = m_viewDir.z();

    u.currentTime = m_time;
    u.color[0] = 0.8F;
    u.color[1] = 0.8F;
    u.color[2] = 0.8F;
    u.color[3] = 1.0F;

    rub->updateDynamicBuffer(m_rhiResources.uniformBuffer, 0, sizeof(UniformData), &u);

    // Update vertex buffer if needed
    if (m_geometryDirty) {
        rub->updateDynamicBuffer(m_rhiResources.vertexBuffer, 0,
                                 m_particles.size() * sizeof(ParticleVertex), m_particles.data());
        m_geometryDirty = false;
    }

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
    cb->draw(MAX_PARTICLES, 1, 0, 0);
}

void SmokeRenderer::initializeRHI(QRhi *rhi) {
    m_rhiResources.vertexBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer,
                                                 MAX_PARTICLES * sizeof(ParticleVertex));
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

void SmokeRenderer::createPipeline(QRhi *rhi) {
    delete m_rhiResources.pipeline;
    m_rhiResources.pipeline = rhi->newGraphicsPipeline();

    QFile vs(":/shaders/shaders/smoke.vert.qsb");
    QFile fs(":/shaders/shaders/smoke.frag.qsb");
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
    layout.setBindings({{sizeof(ParticleVertex)}});
    layout.setAttributes(
        {{0, 0, QRhiVertexInputAttribute::Float, offsetof(ParticleVertex, startLat)},
         {0, 1, QRhiVertexInputAttribute::Float, offsetof(ParticleVertex, startLon)},
         {0, 2, QRhiVertexInputAttribute::Float, offsetof(ParticleVertex, altitude)},
         {0, 3, QRhiVertexInputAttribute::Float, offsetof(ParticleVertex, startTime)},
         {0, 4, QRhiVertexInputAttribute::Float, offsetof(ParticleVertex, isActive)}});
    m_rhiResources.pipeline->setVertexInputLayout(layout);
    m_rhiResources.pipeline->setShaderResourceBindings(m_rhiResources.shaderBindings);

    QRhiRenderTarget *rt = renderTarget();
    if (rt)
        m_rhiResources.pipeline->setRenderPassDescriptor(rt->renderPassDescriptor());

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_rhiResources.pipeline->setTargetBlends({blend});

    m_rhiResources.pipeline->setDepthTest(true);
    m_rhiResources.pipeline->setDepthWrite(false);
    m_rhiResources.pipeline->setTopology(QRhiGraphicsPipeline::Points);

    m_rhiResources.pipeline->create();
}
