#include "SmokeRenderer.h"
#include "Utils.h"
#include <QFile>
#include <QDebug>
#include <QRandomGenerator>

SmokeRenderer::SmokeRenderer() {
    m_particles.resize(MAX_PARTICLES, {0, 0, 0, 0, 0});
}

SmokeRenderer::~SmokeRenderer() { releaseResources(); }

void SmokeRenderer::releaseResources() {
    delete m_pipeline; m_pipeline = nullptr;
    delete m_vertexBuffer; m_vertexBuffer = nullptr;
    delete m_uniformBuffer; m_uniformBuffer = nullptr;
    delete m_bindings; m_bindings = nullptr;
    m_initialized = false;
}

void SmokeRenderer::addSource(float lat, float lon, float altitude) {
    auto* rng = QRandomGenerator::global();
    for (int i = 0; i < PARTICLES_PER_SOURCE; ++i) {
        int idx = m_nextParticleIdx;
        m_particles[idx].startLat = lat;
        m_particles[idx].startLon = lon;
        m_particles[idx].altitude = altitude;
        // Stagger start times: current time + some spread
        m_particles[idx].startTime = m_time + (1500.0f * i / PARTICLES_PER_SOURCE);
        m_particles[idx].isActive = 1.0f;
        
        m_nextParticleIdx = (m_nextParticleIdx + 1) % MAX_PARTICLES;
    }
    m_geometryDirty = true;
}

void SmokeRenderer::render(const RenderState* state) {
    QRhiRenderTarget* rt = renderTarget();
    QRhiCommandBuffer* cb = commandBuffer();
    if (!rt || !cb) return;

    QRhi* r = rt->rhi();
    if (!m_initialized) {
        initializeRHI(r);
        if (!m_initialized) return;
    }

    if (!m_pipeline || m_pipeline->renderPassDescriptor() != rt->renderPassDescriptor()) {
        createPipeline(r);
    }
    if (!m_pipeline) return;

    QRhiResourceUpdateBatch* rub = r->nextResourceUpdateBatch();

    // Update uniform buffer
    UniformData u;
    memset(&u, 0, sizeof(UniformData));
    const float* mData = m_mvp.constData();
    for (int i = 0; i < 16; ++i) u.mvp[i] = mData[i];
    u.color[0] = 0.8f; u.color[1] = 0.8f; u.color[2] = 0.8f; u.color[3] = 1.0f; 
    u.currentTime = m_time;
    u.cameraDistance = m_cameraDistance;
    rub->updateDynamicBuffer(m_uniformBuffer, 0, sizeof(UniformData), &u);

    // Update vertex buffer if needed
    if (m_geometryDirty) {
        rub->updateDynamicBuffer(m_vertexBuffer, 0, m_particles.size() * sizeof(ParticleVertex), m_particles.data());
        m_geometryDirty = false;
    }

    cb->resourceUpdate(rub);

    const QSize rtSize = rt->pixelSize();
    cb->setViewport(QRhiViewport(0, 0, rtSize.width(), rtSize.height()));

    cb->setGraphicsPipeline(m_pipeline);
    const QRhiCommandBuffer::VertexInput vb[] = { { m_vertexBuffer, 0 } };
    cb->setVertexInput(0, 1, vb);
    cb->setShaderResources(m_bindings);
    cb->draw(MAX_PARTICLES, 1, 0, 0);
}

void SmokeRenderer::initializeRHI(QRhi* rhi) {
    m_vertexBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, 
                                     MAX_PARTICLES * sizeof(ParticleVertex));
    m_vertexBuffer->create();

    m_uniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(UniformData));
    m_uniformBuffer->create();

    m_bindings = rhi->newShaderResourceBindings();
    m_bindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_uniformBuffer)
    });
    m_bindings->create();

    createPipeline(rhi);
    m_initialized = true;
}

void SmokeRenderer::createPipeline(QRhi* rhi) {
    delete m_pipeline;
    m_pipeline = rhi->newGraphicsPipeline();

    QFile vs(":/shaders/shaders/smoke.vert.qsb");
    QFile fs(":/shaders/shaders/smoke.frag.qsb");
    QShader v, f;
    if (vs.open(QIODevice::ReadOnly)) { v = QShader::fromSerialized(vs.readAll()); vs.close(); }
    if (fs.open(QIODevice::ReadOnly)) { f = QShader::fromSerialized(fs.readAll()); fs.close(); }

    m_pipeline->setShaderStages({
        { QRhiShaderStage::Vertex, v },
        { QRhiShaderStage::Fragment, f }
    });

    QRhiVertexInputLayout layout;
    layout.setBindings({ { sizeof(ParticleVertex) } });
    layout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float, offsetof(ParticleVertex, startLat) },
        { 0, 1, QRhiVertexInputAttribute::Float, offsetof(ParticleVertex, startLon) },
        { 0, 2, QRhiVertexInputAttribute::Float, offsetof(ParticleVertex, altitude) },
        { 0, 3, QRhiVertexInputAttribute::Float, offsetof(ParticleVertex, startTime) },
        { 0, 4, QRhiVertexInputAttribute::Float, offsetof(ParticleVertex, isActive) }
    });
    m_pipeline->setVertexInputLayout(layout);
    m_pipeline->setShaderResourceBindings(m_bindings);

    QRhiRenderTarget* rt = renderTarget();
    if (rt) m_pipeline->setRenderPassDescriptor(rt->renderPassDescriptor());

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_pipeline->setTargetBlends({ blend });

    m_pipeline->setDepthTest(true);
    m_pipeline->setDepthWrite(false);
    m_pipeline->setTopology(QRhiGraphicsPipeline::Points);

    m_pipeline->create();
}
