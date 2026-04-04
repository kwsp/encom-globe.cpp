#include "PinRenderer.h"
#include "Utils.h"
#include <QFile>
#include <QDebug>

PinRenderer::PinRenderer() = default;

PinRenderer::~PinRenderer() { releaseResources(); }

void PinRenderer::releaseResources() {
    delete m_pipeline;
    m_pipeline = nullptr;
    
    delete m_vertexBuffer;
    m_vertexBuffer = nullptr;
    
    delete m_uniformBuffer;
    m_uniformBuffer = nullptr;
    
    delete m_shaderBindings;
    m_shaderBindings = nullptr;
    
    m_initialized = false;
    m_vertexCapacity = 0;
}

void PinRenderer::render(const RenderState* state) {
    QRhiRenderTarget* rt = renderTarget();
    QRhiCommandBuffer* cb = commandBuffer();
    
    if (!rt || !cb || m_pins.empty())
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

    QRhiResourceUpdateBatch* rub = r->nextResourceUpdateBatch();
    
    // Update MVP uniform
    UniformData ubuf;
    const float* mvpData = m_mvp.constData();
    for (int i = 0; i < 16; ++i) {
        ubuf.mvp[i] = mvpData[i];
    }
    rub->updateDynamicBuffer(m_uniformBuffer, 0, sizeof(UniformData), &ubuf);
    
    // Update vertex buffer if needed
    if (m_geometryDirty || m_vertexBuffer == nullptr) {
        updateBuffers(r, rub);
        m_geometryDirty = false;
    }
    
    cb->resourceUpdate(rub);
    
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
        
        cb->setShaderResources(m_shaderBindings);
        // Draw 2 vertices per pin (line segment)
        cb->draw(m_pins.size() * 2, 1, 0, 0);
    }
}

void PinRenderer::updateBuffers(QRhi* rhi, QRhiResourceUpdateBatch* rub) {
    int requiredVertices = m_pins.size() * 2;
    
    // Reallocate vertex buffer if needed
    if (!m_vertexBuffer || requiredVertices > m_vertexCapacity) {
        if (m_vertexBuffer) {
            delete m_vertexBuffer;
        }
        
        // Add some extra capacity to avoid frequent reallocations
        m_vertexCapacity = requiredVertices + 20; 
        
        m_vertexBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer,
                                      m_vertexCapacity * sizeof(Vertex));
        if (!m_vertexBuffer->create()) {
            qWarning() << "Failed to create pin vertex buffer";
            return;
        }
        
        // Need to recreate pipeline since vertex buffer binding changed
        // Actually, just bindings might be enough, but to be safe:
        m_needsPipelineRebuild = true;
    }
    
    std::vector<Vertex> vertexData;
    vertexData.reserve(requiredVertices);
    
    for (const auto& pin : m_pins) {
        // Start at surface
        QVector3D start = Utils::latLonToXYZ(pin.lat, pin.lon, Utils::GLOBE_RADIUS);
        
        // End at altitude (animated by progress)
        // Progress goes from 0 to 1
        float currentAltitude = 1.0f + (pin.altitude - 1.0f) * pin.progress;
        QVector3D end = Utils::latLonToXYZ(pin.lat, pin.lon, Utils::GLOBE_RADIUS * currentAltitude);
        
        Vertex vStart, vEnd;
        vStart.position[0] = start.x(); vStart.position[1] = start.y(); vStart.position[2] = start.z();
        vEnd.position[0] = end.x(); vEnd.position[1] = end.y(); vEnd.position[2] = end.z();
        
        vStart.color[0] = pin.color.redF(); vStart.color[1] = pin.color.greenF(); vStart.color[2] = pin.color.blueF();
        vEnd.color[0] = pin.color.redF(); vEnd.color[1] = pin.color.greenF(); vEnd.color[2] = pin.color.blueF();
        
        vertexData.push_back(vStart);
        vertexData.push_back(vEnd);
    }
    
    rub->updateDynamicBuffer(m_vertexBuffer, 0, vertexData.size() * sizeof(Vertex), vertexData.data());
}

void PinRenderer::initializeRHI(QRhi* rhi) {
    if (!rhi)
        return;
    
    // Create uniform buffer
    m_uniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 
                                       sizeof(UniformData));
    if (!m_uniformBuffer->create()) {
        qWarning() << "Failed to create pin uniform buffer";
        return;
    }
    
    // Create shader bindings
    m_shaderBindings = rhi->newShaderResourceBindings();
    m_shaderBindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, m_uniformBuffer)
    });
    
    if (!m_shaderBindings->create()) {
        qWarning() << "Failed to create pin shader resource bindings";
        return;
    }
    
    createPipeline(rhi);
    
    m_initialized = true;
}

void PinRenderer::createPipeline(QRhi* rhi) {
    if (m_pipeline) {
        delete m_pipeline;
    }
    m_pipeline = rhi->newGraphicsPipeline();
    
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
    
    m_pipeline->setShaderStages({
        { QRhiShaderStage::Vertex, vertexShader },
        { QRhiShaderStage::Fragment, fragmentShader }
    });
    
    // Vertex input layout: position (vec3), color (vec3)
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { sizeof(Vertex) }
    });
    
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, offsetof(Vertex, position) },
        { 0, 1, QRhiVertexInputAttribute::Float3, offsetof(Vertex, color) }
    });
    
    m_pipeline->setVertexInputLayout(inputLayout);
    m_pipeline->setShaderResourceBindings(m_shaderBindings);
    
    QRhiRenderTarget* rt = renderTarget();
    if (rt) {
        m_pipeline->setRenderPassDescriptor(rt->renderPassDescriptor());
    }
    
    // Enable blending
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_pipeline->setTargetBlends({ blend });
    
    // Enable depth test, but no depth write for transparent lines
    m_pipeline->setDepthTest(true);
    m_pipeline->setDepthWrite(false);
    
    // Draw lines
    m_pipeline->setTopology(QRhiGraphicsPipeline::Lines);
    
    if (!m_pipeline->create()) {
        qWarning() << "Failed to create pin pipeline";
        return;
    }
}

void PinRenderer::setPins(const std::vector<PinData>& pins) {
    m_pins = pins;
    m_geometryDirty = true;
}
