#include "MarkerRenderer.h"
#include "Utils.h"
#include <QFile>
#include <QDebug>
#include <cmath>

MarkerRenderer::MarkerRenderer() = default;

MarkerRenderer::~MarkerRenderer() { releaseResources(); }

void MarkerRenderer::releaseResources() {
    delete m_linePipeline;    m_linePipeline = nullptr;
    delete m_lineVertexBuffer; m_lineVertexBuffer = nullptr;
    delete m_lineUniformBuffer; m_lineUniformBuffer = nullptr;
    delete m_lineBindings;    m_lineBindings = nullptr;

    delete m_spritePipeline;  m_spritePipeline = nullptr;
    delete m_spriteVertexBuffer; m_spriteVertexBuffer = nullptr;
    delete m_spriteUniformBuffer; m_spriteUniformBuffer = nullptr;
    delete m_spriteBindings;  m_spriteBindings = nullptr;

    m_initialized = false;
    m_lineVertexCapacity = 0;
}

void MarkerRenderer::rebuildArcGeometry() {
    m_arcVertices.clear();
    m_arcs.clear();

    for (size_t i = 0; i < m_markers.size(); ++i) {
        const auto& marker = m_markers[i];
        if (marker.previousIndex < 0 || marker.previousIndex >= (int)m_markers.size())
            continue;

        const auto& prev = m_markers[marker.previousIndex];
        ArcConnection arc;
        arc.fromIdx = marker.previousIndex;
        arc.toIdx = (int)i;
        arc.vertexOffset = (int)m_arcVertices.size();
        arc.totalVertices = LINE_SEGMENTS + 1;

        float latDist = (marker.lat - prev.lat) / LINE_SEGMENTS;
        float lonDist = (marker.lon - prev.lon) / LINE_SEGMENTS;

        for (int j = 0; j <= LINE_SEGMENTS; ++j) {
            float t = (float)j / LINE_SEGMENTS;

            // Replicate original encom arc formula
            float rawLat = prev.lat + j * latDist;
            float modLat = std::fmod(90.0f + rawLat, 180.0f) - 90.0f;
            float cosScale = 0.5f + std::cos(j * (5.0f * M_PI / 2.0f) / LINE_SEGMENTS) / 2.0f;
            float nextLat = modLat * cosScale + (j * marker.lat / LINE_SEGMENTS / 2.0f);

            float nextLon = std::fmod(180.0f + prev.lon + j * lonDist, 360.0f) - 180.0f;

            QVector3D pos = Utils::latLonToXYZ(nextLat, nextLon, Utils::GLOBE_RADIUS * 1.2f);

            LineVertex v;
            v.position[0] = pos.x();
            v.position[1] = pos.y();
            v.position[2] = pos.z();
            v.color[0] = marker.color.redF();
            v.color[1] = marker.color.greenF();
            v.color[2] = marker.color.blueF();

            m_arcVertices.push_back(v);
        }

        m_arcs.push_back(arc);
    }
}

void MarkerRenderer::render(const RenderState* state) {
    QRhiRenderTarget* rt = renderTarget();
    QRhiCommandBuffer* cb = commandBuffer();

    if (!rt || !cb || m_markers.empty())
        return;

    QRhi* r = rt->rhi();
    if (!r) return;

    if (!m_initialized) {
        initializeRHI(r);
        if (!m_initialized) return;
    }

    if (m_needsPipelineRebuild || !m_linePipeline ||
        m_linePipeline->renderPassDescriptor() != rt->renderPassDescriptor()) {
        createLinePipeline(r);
        createSpritePipeline(r);
        m_needsPipelineRebuild = false;
    }

    if (!m_linePipeline || !m_spritePipeline)
        return;

    QRhiResourceUpdateBatch* rub = r->nextResourceUpdateBatch();

    // Upload sprite quad vertices once
    if (!m_spriteVertexUploaded && m_spriteVertexBuffer) {
        const float verts[12] = {
            -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
        };
        rub->uploadStaticBuffer(m_spriteVertexBuffer, verts);
        m_spriteVertexUploaded = true;
    }

    // Rebuild arc geometry if dirty
    if (m_geometryDirty) {
        rebuildArcGeometry();

        int needed = (int)m_arcVertices.size();
        if (needed > 0) {
            if (!m_lineVertexBuffer || needed > m_lineVertexCapacity) {
                delete m_lineVertexBuffer;
                m_lineVertexCapacity = needed + 200;
                m_lineVertexBuffer = r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer,
                                                  m_lineVertexCapacity * sizeof(LineVertex));
                m_lineVertexBuffer->create();
            }
            rub->updateDynamicBuffer(m_lineVertexBuffer, 0,
                                     needed * sizeof(LineVertex), m_arcVertices.data());
        }
        m_geometryDirty = false;
    }

    // Update line MVP uniform
    LineUniformData lineUbuf;
    const float* mvpData = m_mvp.constData();
    for (int i = 0; i < 16; ++i) lineUbuf.mvp[i] = mvpData[i];
    rub->updateDynamicBuffer(m_lineUniformBuffer, 0, sizeof(LineUniformData), &lineUbuf);

    // Update sprite uniforms for each marker
    m_spriteUniformCache.resize(m_markers.size());
    for (size_t i = 0; i < m_markers.size(); ++i) {
        const auto& mk = m_markers[i];
        SpriteUniformData& su = m_spriteUniformCache[i];

        QVector3D pos = Utils::latLonToXYZ(mk.lat, mk.lon, Utils::GLOBE_RADIUS * mk.altitude);

        QVector3D toCamera = (m_cameraPos - pos).normalized();
        QVector3D up(0, 1, 0);
        QVector3D right = QVector3D::crossProduct(up, toCamera).normalized();
        if (right.length() < 0.01f) right = QVector3D(1, 0, 0);
        up = QVector3D::crossProduct(toCamera, right).normalized();

        float distToCam = (m_cameraPos - pos).length();
        float scale = 35.0f * m_spriteScale * mk.markerProgress * (distToCam / Utils::CAMERA_DISTANCE);

        QMatrix4x4 model;
        model.setColumn(0, QVector4D(right * scale, 0));
        model.setColumn(1, QVector4D(up * scale, 0));
        model.setColumn(2, QVector4D(toCamera, 0));
        model.setColumn(3, QVector4D(pos, 1));

        QMatrix4x4 spriteMvp = m_mvp * model;
        const float* sm = spriteMvp.constData();
        for (int j = 0; j < 16; ++j) su.mvp[j] = sm[j];

        su.color[0] = mk.color.redF();
        su.color[1] = mk.color.greenF();
        su.color[2] = mk.color.blueF();
        su.padding = 0;

        rub->updateDynamicBuffer(m_spriteUniformBuffer, i * UNIFORM_ALIGNMENT,
                                 sizeof(SpriteUniformData), &su);
    }

    cb->resourceUpdate(rub);

    const QSize rtSize = rt->pixelSize();
    cb->setViewport(QRhiViewport(0, 0, rtSize.width(), rtSize.height()));

    // --- Draw arc lines ---
    if (m_lineVertexBuffer && !m_arcs.empty()) {
        cb->setGraphicsPipeline(m_linePipeline);
        const QRhiCommandBuffer::VertexInput vb[] = { { m_lineVertexBuffer, 0 } };
        cb->setVertexInput(0, 1, vb);
        cb->setShaderResources(m_lineBindings);

        for (const auto& arc : m_arcs) {
            const auto& mk = m_markers[arc.toIdx];
            // Progressive draw: only draw a fraction of the vertices
            int visibleVerts = std::max(2, (int)(arc.totalVertices * mk.lineProgress));
            cb->draw(visibleVerts, 1, arc.vertexOffset, 0);
        }
    }

    // --- Draw marker sprites ---
    cb->setGraphicsPipeline(m_spritePipeline);
    if (m_spriteVertexBuffer) {
        const QRhiCommandBuffer::VertexInput vb[] = { { m_spriteVertexBuffer, 0 } };
        cb->setVertexInput(0, 1, vb);

        for (size_t i = 0; i < m_markers.size(); ++i) {
            if (m_markers[i].markerProgress <= 0.0f) continue;
            QRhiCommandBuffer::DynamicOffset dynOff = { 0, static_cast<quint32>(i * UNIFORM_ALIGNMENT) };
            cb->setShaderResources(m_spriteBindings, 1, &dynOff);
            cb->draw(6, 1, 0, 0);
        }
    }
}

void MarkerRenderer::initializeRHI(QRhi* rhi) {
    if (!rhi) return;

    // Line uniform buffer (single MVP)
    m_lineUniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                                          sizeof(LineUniformData));
    m_lineUniformBuffer->create();

    m_lineBindings = rhi->newShaderResourceBindings();
    m_lineBindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage,
                                                  m_lineUniformBuffer)
    });
    m_lineBindings->create();

    // Sprite resources
    m_spriteVertexBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                           6 * 2 * sizeof(float));
    m_spriteVertexBuffer->create();

    m_spriteUniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                                            UNIFORM_ALIGNMENT * MAX_MARKERS);
    m_spriteUniformBuffer->create();

    m_spriteBindings = rhi->newShaderResourceBindings();
    m_spriteBindings->setBindings({
        QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_spriteUniformBuffer, sizeof(SpriteUniformData))
    });
    m_spriteBindings->create();

    createLinePipeline(rhi);
    createSpritePipeline(rhi);
    m_initialized = true;
}

void MarkerRenderer::createLinePipeline(QRhi* rhi) {
    delete m_linePipeline;
    m_linePipeline = rhi->newGraphicsPipeline();

    // Reuse pin shaders for lines (same format: position + color)
    QFile vs(":/shaders/shaders/pin.vert.qsb");
    QFile fs(":/shaders/shaders/pin.frag.qsb");
    QShader vertShader, fragShader;
    if (vs.open(QIODevice::ReadOnly)) { vertShader = QShader::fromSerialized(vs.readAll()); vs.close(); }
    if (fs.open(QIODevice::ReadOnly)) { fragShader = QShader::fromSerialized(fs.readAll()); fs.close(); }
    if (!vertShader.isValid() || !fragShader.isValid()) return;

    m_linePipeline->setShaderStages({
        { QRhiShaderStage::Vertex, vertShader },
        { QRhiShaderStage::Fragment, fragShader }
    });

    QRhiVertexInputLayout layout;
    layout.setBindings({ { sizeof(LineVertex) } });
    layout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, offsetof(LineVertex, position) },
        { 0, 1, QRhiVertexInputAttribute::Float3, offsetof(LineVertex, color) }
    });
    m_linePipeline->setVertexInputLayout(layout);
    m_linePipeline->setShaderResourceBindings(m_lineBindings);

    QRhiRenderTarget* rt = renderTarget();
    if (rt) m_linePipeline->setRenderPassDescriptor(rt->renderPassDescriptor());

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_linePipeline->setTargetBlends({ blend });

    m_linePipeline->setDepthTest(true);
    m_linePipeline->setDepthWrite(false);
    m_linePipeline->setTopology(QRhiGraphicsPipeline::LineStrip);

    m_linePipeline->create();
}

void MarkerRenderer::createSpritePipeline(QRhi* rhi) {
    delete m_spritePipeline;
    m_spritePipeline = rhi->newGraphicsPipeline();

    // Reuse pintop shaders
    QFile vs(":/shaders/shaders/pintop.vert.qsb");
    QFile fs(":/shaders/shaders/pintop.frag.qsb");
    QShader vertShader, fragShader;
    if (vs.open(QIODevice::ReadOnly)) { vertShader = QShader::fromSerialized(vs.readAll()); vs.close(); }
    if (fs.open(QIODevice::ReadOnly)) { fragShader = QShader::fromSerialized(fs.readAll()); fs.close(); }
    if (!vertShader.isValid() || !fragShader.isValid()) return;

    m_spritePipeline->setShaderStages({
        { QRhiShaderStage::Vertex, vertShader },
        { QRhiShaderStage::Fragment, fragShader }
    });

    QRhiVertexInputLayout layout;
    layout.setBindings({ { 2 * sizeof(float) } });
    layout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 }
    });
    m_spritePipeline->setVertexInputLayout(layout);
    m_spritePipeline->setShaderResourceBindings(m_spriteBindings);

    QRhiRenderTarget* rt = renderTarget();
    if (rt) m_spritePipeline->setRenderPassDescriptor(rt->renderPassDescriptor());

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_spritePipeline->setTargetBlends({ blend });

    m_spritePipeline->setDepthTest(true);
    m_spritePipeline->setDepthWrite(false);

    m_spritePipeline->create();
}

void MarkerRenderer::setMarkers(const std::vector<MarkerData>& markers) {
    m_markers = markers;
    m_geometryDirty = true;
    m_needsPipelineRebuild = false;
}
