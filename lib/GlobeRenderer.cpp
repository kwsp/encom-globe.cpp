#include "GlobeRenderer.h"
#include "Utils.h"
#include <QFile>
#include <QJsonArray>

GlobeRenderer::GlobeRenderer() = default;

GlobeRenderer::~GlobeRenderer() {
    releaseResources();
}

void GlobeRenderer::releaseResources() {
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

void GlobeRenderer::render(const RenderState* state) {
    QRhiRenderTarget* rt = renderTarget();
    QRhiCommandBuffer* cb = commandBuffer();
    
    if (!rt || !cb)
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
    
    if (!m_pipeline || !m_vertexBuffer || m_vertices.empty())
        return;

    // Create resource update batch
    QRhiResourceUpdateBatch* resourceUpdates = r->nextResourceUpdateBatch();
    
    // Update uniform buffer
    updateUniformBuffer(resourceUpdates);
    
    // Update vertex buffer if needed
    if (m_vertexDataChanged && m_vertexBuffer) {
        resourceUpdates->uploadStaticBuffer(m_vertexBuffer, m_vertices.data());
        m_vertexDataChanged = false;
    }
    
    cb->resourceUpdate(resourceUpdates);
    
    // Set up graphics pipeline
    cb->setGraphicsPipeline(m_pipeline);
    
    // Use the render target's actual size for viewport
    const QSize rtSize = rt->pixelSize();
    cb->setViewport(QRhiViewport(0, 0, rtSize.width(), rtSize.height()));
    
    // Bind shader resources
    cb->setShaderResources(m_shaderBindings);
    
    // Bind vertex buffer
    const QRhiCommandBuffer::VertexInput vertexInputs[] = {
        { m_vertexBuffer, 0 }
    };
    cb->setVertexInput(0, 1, vertexInputs);
    
    // Draw
    cb->draw(static_cast<quint32>(m_vertices.size()), 1, 0, 0);
}

void GlobeRenderer::initializeRHI(QRhi* rhi) {
    if (!rhi)
        return;
    
    // Create uniform buffer
    m_uniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 
                                       sizeof(float) * 16 + sizeof(float) * 5); // MVP + uniforms
    if (!m_uniformBuffer->create()) {
        qWarning() << "Failed to create uniform buffer";
        return;
    }
    
    // Create shader bindings
    m_shaderBindings = rhi->newShaderResourceBindings();
    m_shaderBindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_uniformBuffer)
    });
    
    if (!m_shaderBindings->create()) {
        qWarning() << "Failed to create shader resource bindings";
        return;
    }
    
    createPipeline(rhi);
    
    m_initialized = true;
}

void GlobeRenderer::createPipeline(QRhi* rhi) {
    delete m_pipeline;
    m_pipeline = rhi->newGraphicsPipeline();
    
    // Create vertex buffer if we have data and it doesn't exist
    if (!m_vertices.empty() && !m_vertexBuffer) {
        m_vertexBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                           m_vertices.size() * sizeof(Vertex));
        if (!m_vertexBuffer->create()) {
            qWarning() << "Failed to create vertex buffer";
        } else {
            m_vertexDataChanged = true;
        }
    }
    
    // Load shaders from Qt resource system (compiled .qsb files)
    QFile vsFile(":/shaders/shaders/globe.vert.qsb");
    QFile fsFile(":/shaders/shaders/globe.frag.qsb");
    
    QShader vertexShader;
    QShader fragmentShader;
    
    if (vsFile.open(QIODevice::ReadOnly)) {
        vertexShader = QShader::fromSerialized(vsFile.readAll());
        vsFile.close();
    } else {
        qWarning() << "Failed to load vertex shader from" << vsFile.fileName() << vsFile.errorString();
        return;
    }
    
    if (fsFile.open(QIODevice::ReadOnly)) {
        fragmentShader = QShader::fromSerialized(fsFile.readAll());
        fsFile.close();
    } else {
        qWarning() << "Failed to load fragment shader from" << fsFile.fileName() << fsFile.errorString();
        return;
    }
    
    if (!vertexShader.isValid() || !fragmentShader.isValid()) {
        qWarning() << "Shaders are not valid - vertex valid:" << vertexShader.isValid() 
                   << "fragment valid:" << fragmentShader.isValid();
        return;
    }
    
    m_pipeline->setShaderStages({
        { QRhiShaderStage::Vertex, vertexShader },
        { QRhiShaderStage::Fragment, fragmentShader }
    });
    
    // Define vertex input layout
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { sizeof(Vertex) }  // stride
    });
    
    inputLayout.setAttributes({
        // position (location 0)
        { 0, 0, QRhiVertexInputAttribute::Float3, offsetof(Vertex, position) },
        // color (location 1)
        { 0, 1, QRhiVertexInputAttribute::Float3, offsetof(Vertex, color) },
        // longitude (location 2)
        { 0, 2, QRhiVertexInputAttribute::Float, offsetof(Vertex, longitude) }
    });
    
    m_pipeline->setVertexInputLayout(inputLayout);
    m_pipeline->setShaderResourceBindings(m_shaderBindings);
    
    // Use the render target's render pass descriptor
    QRhiRenderTarget* rt = renderTarget();
    if (!rt) {
        qWarning() << "No render target available";
        return;
    }
    m_pipeline->setRenderPassDescriptor(rt->renderPassDescriptor());
    
    // Enable blending for transparency
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_pipeline->setTargetBlends({ blend });
    
    m_pipeline->setDepthTest(true);
    m_pipeline->setDepthWrite(true);
    
    if (!m_pipeline->create()) {
        qWarning() << "Failed to create pipeline";
        return;
    }
}

void GlobeRenderer::updateUniformBuffer(QRhiResourceUpdateBatch* batch) {
    // Pack uniform data: mat4 + 5 floats (with padding for alignment)
    struct UniformData {
        float mvp[16];
        float currentTime;
        float introDuration;
        float introAltitude;
        float cameraDistance;
        float padding;
    } uniformData;
    
    static_assert(sizeof(UniformData) == sizeof(float) * 21, "UniformData size mismatch");
    
    // Copy MVP matrix (column-major)
    const float* mvpData = m_mvp.constData();
    for (int i = 0; i < 16; ++i) {
        uniformData.mvp[i] = mvpData[i];
    }
    
    uniformData.currentTime = m_currentTime;
    uniformData.introDuration = m_introDuration;
    uniformData.introAltitude = m_introAltitude;
    uniformData.cameraDistance = Utils::CAMERA_DISTANCE;
    uniformData.padding = 0.0f;
    
    batch->updateDynamicBuffer(m_uniformBuffer, 0, sizeof(UniformData), &uniformData);
}

void GlobeRenderer::setBaseColor(const QString& color) {
    if (m_baseColor != color) {
        m_baseColor = color;
        if (!m_tileData.isEmpty()) {
            setTileData(m_tileData);
        }
    }
}

void GlobeRenderer::setTileData(const QJsonObject& data) {
    m_tileData = data;
    m_vertices.clear();
    
    QJsonArray tiles = data["tiles"].toArray();
    
    QColor baseColor = Utils::hexToColor(m_baseColor);
    auto palette = Utils::hueSet(baseColor);
    
    for (const QJsonValue& tileVal : tiles) {
        QJsonObject tile = tileVal.toObject();
        
        float lat = tile["lat"].toDouble();
        float lon = tile["lon"].toDouble();
        QJsonArray bounds = tile["b"].toArray();
        
        int numBounds = bounds.size();
        if (numBounds < 3)
            continue;
        
        // Get boundary vertices
        std::vector<QVector3D> boundVerts;
        for (const QJsonValue& bVal : bounds) {
            QJsonObject b = bVal.toObject();
            boundVerts.push_back({ 
                static_cast<float>(b["x"].toDouble()),
                static_cast<float>(b["y"].toDouble()),
                static_cast<float>(b["z"].toDouble())
            });
        }
        
        // Generate triangles: fan from first boundary vertex
        QColor tileColor = Utils::randomTileColor(palette);
        
        for (int i = 1; i < numBounds - 1; ++i) {
            Vertex v0;
            v0.position[0] = boundVerts[0].x();
            v0.position[1] = boundVerts[0].y();
            v0.position[2] = boundVerts[0].z();
            v0.color[0] = tileColor.redF();
            v0.color[1] = tileColor.greenF();
            v0.color[2] = tileColor.blueF();
            v0.longitude = lon;
            
            Vertex v1;
            v1.position[0] = boundVerts[i].x();
            v1.position[1] = boundVerts[i].y();
            v1.position[2] = boundVerts[i].z();
            v1.color[0] = tileColor.redF();
            v1.color[1] = tileColor.greenF();
            v1.color[2] = tileColor.blueF();
            v1.longitude = lon;
            
            Vertex v2;
            v2.position[0] = boundVerts[i+1].x();
            v2.position[1] = boundVerts[i+1].y();
            v2.position[2] = boundVerts[i+1].z();
            v2.color[0] = tileColor.redF();
            v2.color[1] = tileColor.greenF();
            v2.color[2] = tileColor.blueF();
            v2.longitude = lon;
            
            m_vertices.push_back(v0);
            m_vertices.push_back(v1);
            m_vertices.push_back(v2);
        }
    }
    
    m_vertexDataChanged = true;
    m_needsPipelineRebuild = true;  // Force rebuild to create vertex buffer
}

void GlobeRenderer::tick() {
    // Called every frame from Globe::updatePaintNode
}
