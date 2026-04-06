#include "PinRenderer.h"
#include "Utils.h"
#include <QDebug>
#include <QFile>

static const int MAX_PINS = 128;
static const int UNIFORM_ALIGNMENT = 256;

PinRenderer::PinRenderer() = default;

PinRenderer::~PinRenderer() {
    releaseResources();
}

void PinRenderer::releaseResources() {
    m_lineResources.releaseResources();
    m_topResources.releaseResources();

    m_initialized = false;
    m_vertexCapacity = 0;
}

void PinRenderer::render(const RenderState *state) {
    QRhiRenderTarget *rt = renderTarget();
    QRhiCommandBuffer *cb = commandBuffer();

    if (!rt || !cb || m_pins.empty())
        return;

    QRhi *r = rt->rhi();
    if (!r)
        return;

    if (!m_initialized) {
        initializeRHI(r);
        if (!m_initialized)
            return;
    }

    // Rebuild pipeline if render pass changed
    if (m_needsPipelineRebuild || !m_lineResources.pipeline ||
        m_lineResources.pipeline->renderPassDescriptor() != rt->renderPassDescriptor()) {
        createPipeline(r);
        m_needsPipelineRebuild = false;
    }

    if (!m_lineResources.pipeline)
        return;

    QRhiResourceUpdateBatch *rub = r->nextResourceUpdateBatch();

    // First frame: upload top vertex data
    if (!m_topVertexDataUploaded && m_topResources.vertexBuffer) {
        const float vertices[12] = {
            -0.5F, -0.5F, 0.5F, -0.5F, 0.5F, 0.5F, -0.5F, -0.5F, 0.5F, 0.5F, -0.5F, 0.5F,
        };
        rub->uploadStaticBuffer(m_topResources.vertexBuffer, vertices);
        m_topVertexDataUploaded = true;
    }

    // Update MVP uniform for lines
    UniformData ubuf;
    memset(&ubuf, 0, sizeof(UniformData));
    const float *mvpData = m_mvp.constData();
    for (int i = 0; i < 16; ++i) {
        ubuf.mvp[i] = mvpData[i];
    }
    ubuf.viewDir[0] = m_viewDir.x();
    ubuf.viewDir[1] = m_viewDir.y();
    ubuf.viewDir[2] = m_viewDir.z();
    rub->updateDynamicBuffer(m_lineResources.uniformBuffer, 0, sizeof(UniformData), &ubuf);

    // Update top uniforms
    m_topUniformDataCache.resize(m_pins.size());
    for (size_t i = 0; i < m_pins.size(); ++i) {
        const auto &pin = m_pins[i];
        TopUniformData &topUbuf = m_topUniformDataCache[i];
        memset(&topUbuf, 0, sizeof(TopUniformData));

        float animatedProgress = Utils::elasticOut(pin.progress);
        float currentAltitude = 1.0F + (pin.altitude - 1.0F) * animatedProgress;
        QVector3D pos = Utils::latLonToXYZ(pin.lat, pin.lon, Utils::GLOBE_RADIUS * currentAltitude);

        QVector3D toCamera = (m_cameraPos - pos).normalized();
        QVector3D up(0, 1, 0);
        QVector3D right = QVector3D::crossProduct(up, toCamera).normalized();
        if (right.length() < 0.01F) {
            right = QVector3D(1, 0, 0);
        }
        up = QVector3D::crossProduct(toCamera, right).normalized();

        float distToCam = (m_cameraPos - pos).length();

        // Scale pin head smoothly from 0 to 1 during the first 20% of its progress
        // This prevents the pin head from popping into existence instantly
        float headGrowth = std::clamp(pin.progress * 5.0F, 0.0F, 1.0F);

        float scale = 30.0F * m_headScale * headGrowth * (distToCam / Utils::CAMERA_DISTANCE);

        QMatrix4x4 model;
        model.setColumn(0, QVector4D(right * scale, 0));
        model.setColumn(1, QVector4D(up * scale, 0));
        model.setColumn(2, QVector4D(toCamera, 0));
        model.setColumn(3, QVector4D(pos, 1));

        QMatrix4x4 topMvp = m_mvp * model;

        const float *topMvpData = topMvp.constData();
        for (int j = 0; j < 16; ++j) {
            topUbuf.mvp[j] = topMvpData[j];
        }

        topUbuf.color[0] = pin.color.redF();
        topUbuf.color[1] = pin.color.greenF();
        topUbuf.color[2] = pin.color.blueF();

        topUbuf.relativeDepth = QVector3D::dotProduct(pos.normalized(), m_viewDir);

        topUbuf.viewDir[0] = m_viewDir.x();
        topUbuf.viewDir[1] = m_viewDir.y();
        topUbuf.viewDir[2] = m_viewDir.z();

        rub->updateDynamicBuffer(m_topResources.uniformBuffer, i * UNIFORM_ALIGNMENT,
                                 sizeof(TopUniformData), &topUbuf);
    }

    // Update vertex buffer if needed
    if (m_geometryDirty || m_lineResources.vertexBuffer == nullptr) {
        updateBuffers(r, rub);
        m_geometryDirty = false;
    }

    cb->resourceUpdate(rub);

    // Set viewport and scissor
    cb->setViewport(QRhiViewport(m_viewportRect.x(), m_viewportRect.y(), m_viewportRect.width(),
                                 m_viewportRect.height()));
    cb->setScissor(QRhiScissor(m_viewportRect.x(), m_viewportRect.y(), m_viewportRect.width(),
                               m_viewportRect.height()));

    // Bind pipeline
    cb->setGraphicsPipeline(m_lineResources.pipeline);

    // Bind vertex buffer
    if (m_lineResources.vertexBuffer) {
        const QRhiCommandBuffer::VertexInput vb[] = {{m_lineResources.vertexBuffer, 0}};
        cb->setVertexInput(0, 1, vb);

        cb->setShaderResources(m_lineResources.shaderBindings);
        // Draw 2 vertices per pin (line segment)
        cb->draw(m_pins.size() * 2, 1, 0, 0);
    }

    // Draw top sprites
    cb->setGraphicsPipeline(m_topResources.pipeline);
    if (m_topResources.vertexBuffer) {
        const QRhiCommandBuffer::VertexInput vb[] = {{m_topResources.vertexBuffer, 0}};
        cb->setVertexInput(0, 1, vb);

        for (size_t i = 0; i < m_pins.size(); ++i) {
            QRhiCommandBuffer::DynamicOffset dynOff = {0,
                                                       static_cast<quint32>(i * UNIFORM_ALIGNMENT)};
            cb->setShaderResources(m_topResources.shaderBindings, 1, &dynOff);
            cb->draw(6, 1, 0, 0);
        }
    }
}

void PinRenderer::updateBuffers(QRhi *rhi, QRhiResourceUpdateBatch *rub) {
    int requiredVertices = m_pins.size() * 2;

    // Reallocate vertex buffer if needed
    if (!m_lineResources.vertexBuffer || requiredVertices > m_vertexCapacity) {
        if (m_lineResources.vertexBuffer) {
            delete m_lineResources.vertexBuffer;
        }

        // Add some extra capacity to avoid frequent reallocations
        m_vertexCapacity = requiredVertices + 20;

        m_lineResources.vertexBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer,
                                                      m_vertexCapacity * sizeof(Vertex));
        if (!m_lineResources.vertexBuffer->create()) {
            qWarning() << "Failed to create pin vertex buffer";
            return;
        }

        // Need to recreate pipeline since vertex buffer binding changed
        // Actually, just bindings might be enough, but to be safe:
        m_needsPipelineRebuild = true;
    }

    std::vector<Vertex> vertexData;
    vertexData.reserve(requiredVertices);

    for (const auto &pin : m_pins) {
        // Start at surface
        QVector3D start = Utils::latLonToXYZ(pin.lat, pin.lon, Utils::GLOBE_RADIUS);

        // End at altitude (animated by progress)
        // Progress goes from 0 to 1, use elastic out for bounce effect
        float animatedProgress = Utils::elasticOut(pin.progress);
        float currentAltitude = 1.0F + (pin.altitude - 1.0F) * animatedProgress;
        QVector3D end = Utils::latLonToXYZ(pin.lat, pin.lon, Utils::GLOBE_RADIUS * currentAltitude);

        Vertex vStart, vEnd;
        vStart.position[0] = start.x();
        vStart.position[1] = start.y();
        vStart.position[2] = start.z();
        vEnd.position[0] = end.x();
        vEnd.position[1] = end.y();
        vEnd.position[2] = end.z();

        vStart.color[0] = pin.color.redF();
        vStart.color[1] = pin.color.greenF();
        vStart.color[2] = pin.color.blueF();
        vEnd.color[0] = pin.color.redF();
        vEnd.color[1] = pin.color.greenF();
        vEnd.color[2] = pin.color.blueF();

        vertexData.push_back(vStart);
        vertexData.push_back(vEnd);
    }

    rub->updateDynamicBuffer(m_lineResources.vertexBuffer, 0, vertexData.size() * sizeof(Vertex),
                             vertexData.data());
}

void PinRenderer::initializeRHI(QRhi *rhi) {
    if (!rhi)
        return;

    // Create uniform buffer
    m_lineResources.uniformBuffer =
        rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(UniformData));
    if (!m_lineResources.uniformBuffer->create()) {
        qWarning() << "Failed to create pin uniform buffer";
        return;
    }

    // Create shader bindings
    m_lineResources.shaderBindings = rhi->newShaderResourceBindings();
    m_lineResources.shaderBindings->setBindings({QRhiShaderResourceBinding::uniformBuffer(
        0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
        m_lineResources.uniformBuffer)});

    if (!m_lineResources.shaderBindings->create()) {
        qWarning() << "Failed to create pin shader resource bindings";
        return;
    }

    // --- Top Sprite Resources ---

    m_topResources.vertexBuffer =
        rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, 6 * 2 * sizeof(float));
    if (!m_topResources.vertexBuffer->create()) {
        qWarning() << "Failed to create pintop vertex buffer";
        return;
    }

    m_topResources.uniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                                                  UNIFORM_ALIGNMENT * MAX_PINS);
    if (!m_topResources.uniformBuffer->create()) {
        qWarning() << "Failed to create pintop uniform buffer";
        return;
    }

    m_topResources.shaderBindings = rhi->newShaderResourceBindings();
    m_topResources.shaderBindings->setBindings(
        {QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_topResources.uniformBuffer, sizeof(TopUniformData))});

    if (!m_topResources.shaderBindings->create()) {
        qWarning() << "Failed to create pintop shader resource bindings";
        return;
    }

    createPipeline(rhi);

    m_initialized = true;
}

void PinRenderer::createPipeline(QRhi *rhi) {
    if (m_lineResources.pipeline) {
        delete m_lineResources.pipeline;
    }
    m_lineResources.pipeline = rhi->newGraphicsPipeline();

    // Load shaders
    QFile vsFile(":/shaders/shaders/pin.vert.qsb");
    QFile fsFile(":/shaders/shaders/pin.frag.qsb");

    QShader vertexShader;
    QShader fragmentShader;

    if (vsFile.open(QIODevice::ReadOnly)) {
        vertexShader = QShader::fromSerialized(vsFile.readAll());
        vsFile.close();
    } else {
        qWarning() << "Failed to load pin vertex shader";
        return;
    }

    if (fsFile.open(QIODevice::ReadOnly)) {
        fragmentShader = QShader::fromSerialized(fsFile.readAll());
        fsFile.close();
    } else {
        qWarning() << "Failed to load pin fragment shader";
        return;
    }

    m_lineResources.pipeline->setShaderStages(
        {{QRhiShaderStage::Vertex, vertexShader}, {QRhiShaderStage::Fragment, fragmentShader}});

    // Vertex input layout: position (vec3), color (vec3)
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({{sizeof(Vertex)}});

    inputLayout.setAttributes({{0, 0, QRhiVertexInputAttribute::Float3, offsetof(Vertex, position)},
                               {0, 1, QRhiVertexInputAttribute::Float3, offsetof(Vertex, color)}});

    m_lineResources.pipeline->setVertexInputLayout(inputLayout);
    m_lineResources.pipeline->setShaderResourceBindings(m_lineResources.shaderBindings);

    QRhiRenderTarget *rt = renderTarget();
    if (rt) {
        m_lineResources.pipeline->setRenderPassDescriptor(rt->renderPassDescriptor());
    }

    // Enable blending
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_lineResources.pipeline->setTargetBlends({blend});

    // Enable depth test, but no depth write for transparent lines
    m_lineResources.pipeline->setDepthTest(true);
    m_lineResources.pipeline->setDepthWrite(false);

    // Draw lines
    m_lineResources.pipeline->setTopology(QRhiGraphicsPipeline::Lines);
    m_lineResources.pipeline->setLineWidth(2.0F);

    if (!m_lineResources.pipeline->create()) {
        qWarning() << "Failed to create pin pipeline";
        return;
    }

    // --- Top Sprite Pipeline ---
    if (m_topResources.pipeline) {
        delete m_topResources.pipeline;
    }
    m_topResources.pipeline = rhi->newGraphicsPipeline();

    QFile topVsFile(":/shaders/shaders/pintop.vert.qsb");
    QFile topFsFile(":/shaders/shaders/pintop.frag.qsb");

    QShader topVertexShader;
    QShader topFragmentShader;

    if (topVsFile.open(QIODevice::ReadOnly)) {
        topVertexShader = QShader::fromSerialized(topVsFile.readAll());
        topVsFile.close();
    } else {
        qWarning() << "Failed to load pintop vertex shader";
        return;
    }

    if (topFsFile.open(QIODevice::ReadOnly)) {
        topFragmentShader = QShader::fromSerialized(topFsFile.readAll());
        topFsFile.close();
    } else {
        qWarning() << "Failed to load pintop fragment shader";
        return;
    }

    m_topResources.pipeline->setShaderStages({{QRhiShaderStage::Vertex, topVertexShader},
                                              {QRhiShaderStage::Fragment, topFragmentShader}});

    QRhiVertexInputLayout topInputLayout;
    topInputLayout.setBindings({{sizeof(TopVertex)}});

    topInputLayout.setAttributes(
        {{0, 0, QRhiVertexInputAttribute::Float2, offsetof(TopVertex, offset)}});

    m_topResources.pipeline->setVertexInputLayout(topInputLayout);
    m_topResources.pipeline->setShaderResourceBindings(m_topResources.shaderBindings);

    if (rt) {
        m_topResources.pipeline->setRenderPassDescriptor(rt->renderPassDescriptor());
    }

    m_topResources.pipeline->setTargetBlends({blend}); // same blend as lines
    m_topResources.pipeline->setDepthTest(true);
    m_topResources.pipeline->setDepthWrite(false);

    if (!m_topResources.pipeline->create()) {
        qWarning() << "Failed to create pintop pipeline";
        return;
    }
}

void PinRenderer::setPins(const std::vector<PinData> &pins) {
    m_pins = pins;
    m_geometryDirty = true;
}
