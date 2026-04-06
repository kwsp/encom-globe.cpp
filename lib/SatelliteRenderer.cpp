#include "SatelliteRenderer.h"
#include "Utils.h"
#include <QDebug>
#include <QFile>

static const int MAX_SATELLITES = 64;
static const int UNIFORM_ALIGNMENT = 256; // Typical GPU alignment for dynamic uniforms

SatelliteRenderer::SatelliteRenderer() = default;

SatelliteRenderer::~SatelliteRenderer() {
    releaseResources();
}

void SatelliteRenderer::releaseResources() {
    m_rhiResources.releaseResources();
    m_initialized = false;
}

void SatelliteRenderer::render(const RenderState *state) {
    QRhiRenderTarget *rt = renderTarget();
    QRhiCommandBuffer *cb = commandBuffer();

    if (!rt || !cb || m_satellites.empty())
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
    if (m_needsPipelineRebuild || !m_rhiResources.pipeline ||
        m_rhiResources.pipeline->renderPassDescriptor() != rt->renderPassDescriptor()) {
        createPipeline(r);
        m_needsPipelineRebuild = false;
    }

    if (!m_rhiResources.pipeline)
        return;

    // First frame: upload vertex data
    if (!m_vertexDataUploaded && m_rhiResources.vertexBuffer) {
        const float vertices[12] = {
            -0.5F, -0.5F,                            // Triangle 1
            0.5F,  -0.5F, 0.5F,  0.5F, -0.5F, -0.5F, // Triangle 2
            0.5F,  0.5F,  -0.5F, 0.5F,
        };
        QRhiResourceUpdateBatch *rub = r->nextResourceUpdateBatch();
        rub->uploadStaticBuffer(m_rhiResources.vertexBuffer, vertices);
        cb->resourceUpdate(rub);
        m_vertexDataUploaded = true;
    }

    // Prepare uniform data for all satellites
    m_uniformDataCache.resize(m_satellites.size());
    for (size_t i = 0; i < m_satellites.size(); ++i) {
        const auto &sat = m_satellites[i];
        UniformData &uniformData = m_uniformDataCache[i];
        memset(&uniformData, 0, sizeof(UniformData));

        // Calculate satellite position
        QVector3D pos = Utils::latLonToXYZ(sat.lat, sat.lon, Utils::GLOBE_RADIUS * sat.altitude);

        // Build a billboard matrix that faces the camera
        QVector3D toCamera = (m_cameraPos - pos).normalized();
        QVector3D up(0, 1, 0);
        QVector3D right = QVector3D::crossProduct(up, toCamera).normalized();
        if (right.length() < 0.01F) {
            right = QVector3D(1, 0, 0);
        }
        up = QVector3D::crossProduct(toCamera, right).normalized();

        // Scale based on distance, with intro growth animation
        float distToCam = (m_cameraPos - pos).length();
        float introScale = sat.progress * sat.progress; // quadratic ease-in
        float scale = 150.0F * sat.size * introScale * (distToCam / Utils::CAMERA_DISTANCE);

        // Build model matrix
        QMatrix4x4 model;
        model.setColumn(0, QVector4D(right * scale, 0));
        model.setColumn(1, QVector4D(up * scale, 0));
        model.setColumn(2, QVector4D(toCamera, 0));
        model.setColumn(3, QVector4D(pos, 1));

        QMatrix4x4 mvp = m_mvp * model;

        // Copy MVP
        const float *mvpData = mvp.constData();
        for (int j = 0; j < 16; ++j) {
            uniformData.mvp[j] = mvpData[j];
        }

        // Position
        uniformData.position[0] = pos.x();
        uniformData.position[1] = pos.y();
        uniformData.position[2] = pos.z();

        // Animation
        uniformData.time = m_time;
        uniformData.size = sat.size;

        uniformData.viewDir[0] = m_viewDir.x();
        uniformData.viewDir[1] = m_viewDir.y();
        uniformData.viewDir[2] = m_viewDir.z();

        // Wave color
        uniformData.waveColor[0] = sat.waveColor.redF();
        uniformData.waveColor[1] = sat.waveColor.greenF();
        uniformData.waveColor[2] = sat.waveColor.blueF();

        // Calculate arc angle pointing toward globe center
        // Direction from satellite to globe center
        QVector3D toGlobe = (-pos).normalized();

        // Project onto billboard's 2D plane
        // Billboard's X axis = right, Y axis = up
        // But shader UV has Y inverted (0 at top, 1 at bottom)
        float localX = QVector3D::dotProduct(toGlobe, right);
        float localY = QVector3D::dotProduct(toGlobe, up);

        // In shader: uv.y = 0 at top, 1 at bottom
        // After uv*2-1: -1 at top, +1 at bottom
        // So shader +Y = down = -up in world space
        uniformData.arcAngle = atan2(-localY, localX);

        // Core color (satellite color from QML)
        uniformData.coreColor[0] = sat.coreColor.redF();
        uniformData.coreColor[1] = sat.coreColor.greenF();
        uniformData.coreColor[2] = sat.coreColor.blueF();
    }

    // Upload ALL uniform data in ONE batch before drawing
    QRhiResourceUpdateBatch *rub = r->nextResourceUpdateBatch();
    for (size_t i = 0; i < m_uniformDataCache.size(); ++i) {
        rub->updateDynamicBuffer(m_rhiResources.uniformBuffer, i * UNIFORM_ALIGNMENT,
                                 sizeof(UniformData), &m_uniformDataCache[i]);
    }
    cb->resourceUpdate(rub);

    // Set viewport and scissor
    cb->setViewport(QRhiViewport(m_viewportRect.x(), m_viewportRect.y(), m_viewportRect.width(),
                                 m_viewportRect.height()));
    cb->setScissor(QRhiScissor(m_viewportRect.x(), m_viewportRect.y(), m_viewportRect.width(),
                               m_viewportRect.height()));

    // Bind pipeline
    cb->setGraphicsPipeline(m_rhiResources.pipeline);

    // Bind vertex buffer
    if (m_rhiResources.vertexBuffer) {
        const QRhiCommandBuffer::VertexInput vb[] = {{m_rhiResources.vertexBuffer, 0}};
        cb->setVertexInput(0, 1, vb);
    }

    // Draw each satellite with dynamic offset
    for (size_t i = 0; i < m_satellites.size(); ++i) {
        QRhiCommandBuffer::DynamicOffset dynOff = {0, static_cast<quint32>(i * UNIFORM_ALIGNMENT)};
        cb->setShaderResources(m_rhiResources.shaderBindings, 1, &dynOff);
        cb->draw(6, 1, 0, 0);
    }
}

void SatelliteRenderer::initializeRHI(QRhi *rhi) {
    if (!rhi)
        return;

    // Create vertex buffer (6 vertices * 2 floats * 4 bytes = 48 bytes)
    m_rhiResources.vertexBuffer =
        rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, 6 * 2 * sizeof(float));
    if (!m_rhiResources.vertexBuffer->create()) {
        qWarning() << "Failed to create satellite vertex buffer";
        return;
    }

    // Create uniform buffer with space for all satellites
    // Use UNIFORM_ALIGNMENT * MAX_SATELLITES for buffer size
    m_rhiResources.uniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                                                  UNIFORM_ALIGNMENT * MAX_SATELLITES);
    if (!m_rhiResources.uniformBuffer->create()) {
        qWarning() << "Failed to create satellite uniform buffer";
        return;
    }

    // Create shader bindings with dynamic offset
    m_rhiResources.shaderBindings = rhi->newShaderResourceBindings();
    m_rhiResources.shaderBindings->setBindings(
        {QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_rhiResources.uniformBuffer, sizeof(UniformData))});

    if (!m_rhiResources.shaderBindings->create()) {
        qWarning() << "Failed to create satellite shader resource bindings";
        return;
    }

    createPipeline(rhi);

    m_initialized = true;
}

void SatelliteRenderer::createPipeline(QRhi *rhi) {
    delete m_rhiResources.pipeline;
    m_rhiResources.pipeline = rhi->newGraphicsPipeline();

    // Load shaders
    auto [vertexShader, fragmentShader] =
        RhiResources::loadShaders(":/shaders/shaders/satellite.vert.qsb", ":/shaders/shaders/satellite.frag.qsb");

    m_rhiResources.pipeline->setShaderStages(
        {{QRhiShaderStage::Vertex, vertexShader}, {QRhiShaderStage::Fragment, fragmentShader}});

    // Vertex input layout
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({{sizeof(Vertex)}});

    inputLayout.setAttributes({{0, 0, QRhiVertexInputAttribute::Float2, offsetof(Vertex, offset)}});

    m_rhiResources.pipeline->setVertexInputLayout(inputLayout);
    m_rhiResources.pipeline->setShaderResourceBindings(m_rhiResources.shaderBindings);

    QRhiRenderTarget *rt = renderTarget();
    if (rt) {
        m_rhiResources.pipeline->setRenderPassDescriptor(rt->renderPassDescriptor());
    } else {
        qWarning() << "No render target for satellite pipeline";
        return;
    }

    // Enable blending
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_rhiResources.pipeline->setTargetBlends({blend});

    // No depth for billboards
    m_rhiResources.pipeline->setDepthTest(false);
    m_rhiResources.pipeline->setDepthWrite(false);

    if (!m_rhiResources.pipeline->create()) {
        qWarning() << "Failed to create satellite pipeline";
        return;
    }
}

void SatelliteRenderer::setSatellites(const std::vector<SatelliteData> &satellites) {
    m_satellites = satellites;
    m_needsPipelineRebuild = true;
    m_vertexDataUploaded = false;
}

void SatelliteRenderer::clearSatellites() {
    m_satellites.clear();
    m_uniformDataCache.clear();
    m_needsPipelineRebuild = true;
}
