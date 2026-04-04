#include "SatelliteRenderer.h"
#include "Utils.h"
#include <QFile>
#include <QDebug>

SatelliteRenderer::SatelliteRenderer() = default;

SatelliteRenderer::~SatelliteRenderer() {
    releaseResources();
}

void SatelliteRenderer::releaseResources() {
    delete m_pipeline;
    m_pipeline = nullptr;
    
    delete m_vertexBuffer;
    m_vertexBuffer = nullptr;
    
    delete m_uniformBuffer;
    m_uniformBuffer = nullptr;
    
    delete m_shaderBindings;
    m_shaderBindings = nullptr;
    
    m_initialized = false;
}

void SatelliteRenderer::render(const RenderState* state) {
    QRhiRenderTarget* rt = renderTarget();
    QRhiCommandBuffer* cb = commandBuffer();
    
    if (!rt || !cb || m_satellites.empty())
        return;
    
    QRhi* r = rt->rhi();
    if (!r)
        return;
    
    if (!m_initialized) {
        initializeRHI(r);
        if (!m_initialized)
            return;
    }
    
    // Rebuild pipeline if render pass changed
    if (m_needsPipelineRebuild || !m_pipeline || 
        m_pipeline->renderPassDescriptor() != rt->renderPassDescriptor()) {
        createPipeline(r);
        m_needsPipelineRebuild = false;
    }
    
    if (!m_pipeline)
        return;

    // First frame: upload vertex data
    if (!m_vertexDataUploaded && m_vertexBuffer) {
        const float vertices[12] = {
            -0.5f, -0.5f,  // Triangle 1
             0.5f, -0.5f,
             0.5f,  0.5f,
            -0.5f, -0.5f,  // Triangle 2
             0.5f,  0.5f,
            -0.5f,  0.5f,
        };
        QRhiResourceUpdateBatch* rub = r->nextResourceUpdateBatch();
        rub->uploadStaticBuffer(m_vertexBuffer, vertices);
        cb->resourceUpdate(rub);
        m_vertexDataUploaded = true;
    }
    
    // Set viewport
    const QSize rtSize = rt->pixelSize();
    cb->setViewport(QRhiViewport(0, 0, rtSize.width(), rtSize.height()));
    
    // Bind pipeline
    cb->setGraphicsPipeline(m_pipeline);
    
    // Bind vertex buffer
    if (m_vertexBuffer) {
        const QRhiCommandBuffer::VertexInput vb[] = {
            { m_vertexBuffer, 0 }
        };
        cb->setVertexInput(0, 1, vb);
    }
    
    // Draw each satellite - update uniform and draw one at a time
    for (size_t i = 0; i < m_satellites.size(); ++i) {
        const auto& sat = m_satellites[i];
        
        // Update uniform buffer directly
        UniformData uniformData;
        memset(&uniformData, 0, sizeof(UniformData));
        
        // Calculate satellite position
        QVector3D pos = Utils::latLonToXYZ(sat.lat, sat.lon, Utils::GLOBE_RADIUS * sat.altitude);
        
        // Build a billboard matrix that faces the camera
        QVector3D toCamera = (m_cameraPos - pos).normalized();
        QVector3D up(0, 1, 0);
        QVector3D right = QVector3D::crossProduct(up, toCamera).normalized();
        if (right.length() < 0.01f) {
            right = QVector3D(1, 0, 0);
        }
        up = QVector3D::crossProduct(toCamera, right).normalized();
        
        // Scale based on distance
        float distToCam = (m_cameraPos - pos).length();
        float scale = 100.0f * sat.size * (distToCam / Utils::CAMERA_DISTANCE);
        
        // Build model matrix
        QMatrix4x4 model;
        model.setColumn(0, QVector4D(right * scale, 0));
        model.setColumn(1, QVector4D(up * scale, 0));
        model.setColumn(2, QVector4D(toCamera, 0));
        model.setColumn(3, QVector4D(pos, 1));
        
        QMatrix4x4 mvp = m_mvp * model;
        
        // Copy MVP
        const float* mvpData = mvp.constData();
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
        
        // Wave color
        uniformData.waveColor[0] = sat.waveColor.redF();
        uniformData.waveColor[1] = sat.waveColor.greenF();
        uniformData.waveColor[2] = sat.waveColor.blueF();
        
        // Update and draw
        QRhiResourceUpdateBatch* rub = r->nextResourceUpdateBatch();
        rub->updateDynamicBuffer(m_uniformBuffer, 0, sizeof(UniformData), &uniformData);
        cb->resourceUpdate(rub);
        
        cb->setShaderResources(m_shaderBindings);
        cb->draw(6, 1, 0, 0);
    }
}

void SatelliteRenderer::initializeRHI(QRhi* rhi) {
    if (!rhi)
        return;
    
    // Create vertex buffer (6 vertices * 2 floats * 4 bytes = 48 bytes)
    m_vertexBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                      6 * 2 * sizeof(float));
    if (!m_vertexBuffer->create()) {
        qWarning() << "Failed to create satellite vertex buffer";
        return;
    }
    
    // Create uniform buffer
    m_uniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 
                                       sizeof(UniformData));
    if (!m_uniformBuffer->create()) {
        qWarning() << "Failed to create satellite uniform buffer";
        return;
    }
    
    // Create shader bindings
    m_shaderBindings = rhi->newShaderResourceBindings();
    m_shaderBindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | 
                                                     QRhiShaderResourceBinding::FragmentStage, 
                                                  m_uniformBuffer)
    });
    
    if (!m_shaderBindings->create()) {
        qWarning() << "Failed to create satellite shader resource bindings";
        return;
    }
    
    createPipeline(rhi);
    
    m_initialized = true;
}

void SatelliteRenderer::createPipeline(QRhi* rhi) {
    delete m_pipeline;
    m_pipeline = rhi->newGraphicsPipeline();
    
    // Load shaders
    QFile vsFile(":/shaders/shaders/satellite.vert.qsb");
    QFile fsFile(":/shaders/shaders/satellite.frag.qsb");
    
    QShader vertexShader;
    QShader fragmentShader;
    
    if (vsFile.open(QIODevice::ReadOnly)) {
        vertexShader = QShader::fromSerialized(vsFile.readAll());
        vsFile.close();
    } else {
        qWarning() << "Failed to load satellite vertex shader:" << vsFile.errorString();
        return;
    }
    
    if (fsFile.open(QIODevice::ReadOnly)) {
        fragmentShader = QShader::fromSerialized(fsFile.readAll());
        fsFile.close();
    } else {
        qWarning() << "Failed to load satellite fragment shader:" << fsFile.errorString();
        return;
    }
    
    if (!vertexShader.isValid() || !fragmentShader.isValid()) {
        qWarning() << "Satellite shaders are not valid";
        return;
    }
    
    m_pipeline->setShaderStages({
        { QRhiShaderStage::Vertex, vertexShader },
        { QRhiShaderStage::Fragment, fragmentShader }
    });
    
    // Vertex input layout
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { sizeof(Vertex) }
    });
    
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, offsetof(Vertex, offset) }
    });
    
    m_pipeline->setVertexInputLayout(inputLayout);
    m_pipeline->setShaderResourceBindings(m_shaderBindings);
    
    QRhiRenderTarget* rt = renderTarget();
    if (rt) {
        m_pipeline->setRenderPassDescriptor(rt->renderPassDescriptor());
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
    m_pipeline->setTargetBlends({ blend });
    
    // No depth for billboards
    m_pipeline->setDepthTest(false);
    m_pipeline->setDepthWrite(false);
    
    if (!m_pipeline->create()) {
        qWarning() << "Failed to create satellite pipeline";
        return;
    }
}

void SatelliteRenderer::updateUniformBuffer(QRhiResourceUpdateBatch* batch, const SatelliteData& sat) {
    UniformData uniformData;
    memset(&uniformData, 0, sizeof(UniformData));
    
    // Calculate satellite position
    QVector3D pos = Utils::latLonToXYZ(sat.lat, sat.lon, Utils::GLOBE_RADIUS * sat.altitude);
    
    // Build a billboard matrix that faces the camera
    QVector3D toCamera = (m_cameraPos - pos).normalized();
    
    // Handle edge case when looking straight down
    QVector3D up(0, 1, 0);
    QVector3D right = QVector3D::crossProduct(up, toCamera).normalized();
    if (right.length() < 0.01f) {
        right = QVector3D(1, 0, 0);
    }
    up = QVector3D::crossProduct(toCamera, right).normalized();
    
    // Scale based on distance for consistent screen size
    float distToCam = (m_cameraPos - pos).length();
    float scale = 100.0f * sat.size * (distToCam / Utils::CAMERA_DISTANCE);
    
    // Build model matrix
    QMatrix4x4 model;
    model.setColumn(0, QVector4D(right * scale, 0));
    model.setColumn(1, QVector4D(up * scale, 0));
    model.setColumn(2, QVector4D(toCamera, 0));
    model.setColumn(3, QVector4D(pos, 1));
    
    QMatrix4x4 mvp = m_mvp * model;
    
    // Copy MVP
    const float* mvpData = mvp.constData();
    for (int i = 0; i < 16; ++i) {
        uniformData.mvp[i] = mvpData[i];
    }
    
    // Position
    uniformData.position[0] = pos.x();
    uniformData.position[1] = pos.y();
    uniformData.position[2] = pos.z();
    
    // Animation
    uniformData.time = m_time;
    uniformData.size = sat.size;
    
    // Wave color
    uniformData.waveColor[0] = sat.waveColor.redF();
    uniformData.waveColor[1] = sat.waveColor.greenF();
    uniformData.waveColor[2] = sat.waveColor.blueF();
    
    batch->updateDynamicBuffer(m_uniformBuffer, 0, sizeof(UniformData), &uniformData);
}

void SatelliteRenderer::setSatellites(const std::vector<SatelliteData>& satellites) {
    m_satellites = satellites;
    m_needsPipelineRebuild = true;
    m_vertexDataUploaded = false;
}

void SatelliteRenderer::clearSatellites() {
    m_satellites.clear();
    m_needsPipelineRebuild = true;
}
