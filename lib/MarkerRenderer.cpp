#include "MarkerRenderer.h"
#include "Utils.h"
#include <QDebug>
#include <QFile>
#include <cmath>

MarkerRenderer::MarkerRenderer() = default;

MarkerRenderer::~MarkerRenderer() {
    releaseResources();
}

void MarkerRenderer::releaseResources() {
    m_lineResources.releaseResources();
    m_spriteResources.releaseResources();

    m_initialized = false;
    m_lineVertexCapacity = 0;
}

void MarkerRenderer::rebuildArcGeometry() {
    m_arcVertices.clear();
    m_arcs.clear();

    for (size_t i = 0; i < m_markers.size(); ++i) {
        const auto &marker = m_markers[i];
        if (marker.previousIndex < 0 || marker.previousIndex >= (int)m_markers.size())
            continue;

        const auto &prev = m_markers[marker.previousIndex];
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
            float modLat = std::fmod(90.0F + rawLat, 180.0F) - 90.0F;
            float cosScale = 0.5F + std::cos(j * (5.0F * M_PI / 2.0F) / LINE_SEGMENTS) / 2.0F;
            float nextLat = modLat * cosScale + (j * marker.lat / LINE_SEGMENTS / 2.0F);

            float nextLon = std::fmod(180.0F + prev.lon + j * lonDist, 360.0F) - 180.0F;

            QVector3D pos = Utils::latLonToXYZ(nextLat, nextLon, Utils::GLOBE_RADIUS * 1.2F);

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

void MarkerRenderer::render(const RenderState *state) {
    QRhiRenderTarget *rt = renderTarget();
    QRhiCommandBuffer *cb = commandBuffer();

    if (!rt || !cb || m_markers.empty())
        return;

    QRhi *r = rt->rhi();
    if (!r)
        return;

    if (!m_initialized) {
        initializeRHI(r);
        if (!m_initialized)
            return;
    }

    if (m_needsPipelineRebuild || !m_lineResources.pipeline ||
        m_lineResources.pipeline->renderPassDescriptor() != rt->renderPassDescriptor()) {
        createLinePipeline(r);
        createSpritePipeline(r);
        m_needsPipelineRebuild = false;
    }

    if (!m_lineResources.pipeline || !m_spriteResources.pipeline)
        return;

    QRhiResourceUpdateBatch *rub = r->nextResourceUpdateBatch();

    // Upload sprite quad vertices once
    if (!m_spriteVertexUploaded && m_spriteResources.vertexBuffer) {
        const float verts[12] = {
            -0.5F, -0.5F, 0.5F, -0.5F, 0.5F, 0.5F, -0.5F, -0.5F, 0.5F, 0.5F, -0.5F, 0.5F,
        };
        rub->uploadStaticBuffer(m_spriteResources.vertexBuffer, verts);
        m_spriteVertexUploaded = true;
    }

    // Rebuild arc geometry if dirty
    if (m_geometryDirty) {
        rebuildArcGeometry();

        int needed = (int)m_arcVertices.size();
        if (needed > 0) {
            if (!m_lineResources.vertexBuffer || needed > m_lineVertexCapacity) {
                delete m_lineResources.vertexBuffer;
                m_lineVertexCapacity = needed + 200;
                m_lineResources.vertexBuffer =
                    r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer,
                                 m_lineVertexCapacity * sizeof(LineVertex));
                m_lineResources.vertexBuffer->create();
            }
            rub->updateDynamicBuffer(m_lineResources.vertexBuffer, 0, needed * sizeof(LineVertex),
                                     m_arcVertices.data());
        }
        m_geometryDirty = false;
    }

    // Update line MVP uniform
    LineUniformData lineUbuf;
    memset(&lineUbuf, 0, sizeof(LineUniformData));
    const float *mvpData = m_mvp.constData();
    for (int i = 0; i < 16; ++i)
        lineUbuf.mvp[i] = mvpData[i];

    lineUbuf.viewDir[0] = m_viewDir.x();
    lineUbuf.viewDir[1] = m_viewDir.y();
    lineUbuf.viewDir[2] = m_viewDir.z();

    rub->updateDynamicBuffer(m_lineResources.uniformBuffer, 0, sizeof(LineUniformData), &lineUbuf);

    // Update sprite uniforms for each marker
    m_spriteUniformCache.resize(m_markers.size());
    for (size_t i = 0; i < m_markers.size(); ++i) {
        const auto &mk = m_markers[i];
        SpriteUniformData &su = m_spriteUniformCache[i];
        memset(&su, 0, sizeof(SpriteUniformData));

        QVector3D pos = Utils::latLonToXYZ(mk.lat, mk.lon, Utils::GLOBE_RADIUS * mk.altitude);

        QVector3D toCamera = (m_cameraPos - pos).normalized();
        QVector3D up(0, 1, 0);
        QVector3D right = QVector3D::crossProduct(up, toCamera).normalized();
        if (right.length() < 0.01F)
            right = QVector3D(1, 0, 0);
        up = QVector3D::crossProduct(toCamera, right).normalized();

        float distToCam = (m_cameraPos - pos).length();
        float scale =
            35.0F * m_spriteScale * mk.markerProgress * (distToCam / Utils::CAMERA_DISTANCE);

        QMatrix4x4 model;
        model.setColumn(0, QVector4D(right * scale, 0));
        model.setColumn(1, QVector4D(up * scale, 0));
        model.setColumn(2, QVector4D(toCamera, 0));
        model.setColumn(3, QVector4D(pos, 1));

        QMatrix4x4 spriteMvp = m_mvp * model;
        const float *sm = spriteMvp.constData();
        for (int j = 0; j < 16; ++j)
            su.mvp[j] = sm[j];

        su.color[0] = mk.color.redF();
        su.color[1] = mk.color.greenF();
        su.color[2] = mk.color.blueF();
        su.relativeDepth = QVector3D::dotProduct(pos.normalized(), m_viewDir);

        rub->updateDynamicBuffer(m_spriteResources.uniformBuffer, i * UNIFORM_ALIGNMENT,
                                 sizeof(SpriteUniformData), &su);
    }

    cb->resourceUpdate(rub);

    // Set viewport and scissor
    cb->setViewport(QRhiViewport(m_viewportRect.x(), m_viewportRect.y(), m_viewportRect.width(),
                                 m_viewportRect.height()));
    cb->setScissor(QRhiScissor(m_viewportRect.x(), m_viewportRect.y(), m_viewportRect.width(),
                               m_viewportRect.height()));

    const QSize rtSize = rt->pixelSize();
    // (removed redundant setViewport from here)

    // --- Draw arc lines ---
    if (m_lineResources.vertexBuffer && !m_arcs.empty()) {
        cb->setGraphicsPipeline(m_lineResources.pipeline);
        const QRhiCommandBuffer::VertexInput vb[] = {{m_lineResources.vertexBuffer, 0}};
        cb->setVertexInput(0, 1, vb);
        cb->setShaderResources(m_lineResources.shaderBindings);

        for (const auto &arc : m_arcs) {
            const auto &mk = m_markers[arc.toIdx];
            // Progressive draw: only draw a fraction of the vertices
            int visibleVerts = std::max(2, (int)(arc.totalVertices * mk.lineProgress));
            cb->draw(visibleVerts, 1, arc.vertexOffset, 0);
        }
    }

    // --- Draw marker sprites ---
    cb->setGraphicsPipeline(m_spriteResources.pipeline);
    if (m_spriteResources.vertexBuffer) {
        const QRhiCommandBuffer::VertexInput vb[] = {{m_spriteResources.vertexBuffer, 0}};
        cb->setVertexInput(0, 1, vb);

        for (size_t i = 0; i < m_markers.size(); ++i) {
            if (m_markers[i].markerProgress <= 0.0F)
                continue;
            QRhiCommandBuffer::DynamicOffset dynOff = {0,
                                                       static_cast<quint32>(i * UNIFORM_ALIGNMENT)};
            cb->setShaderResources(m_spriteResources.shaderBindings, 1, &dynOff);
            cb->draw(6, 1, 0, 0);
        }
    }
}

void MarkerRenderer::initializeRHI(QRhi *rhi) {
    if (!rhi)
        return;

    // Line uniform buffer (single MVP)
    m_lineResources.uniformBuffer =
        rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(LineUniformData));
    m_lineResources.uniformBuffer->create();

    m_lineResources.shaderBindings = rhi->newShaderResourceBindings();
    m_lineResources.shaderBindings->setBindings({QRhiShaderResourceBinding::uniformBuffer(
        0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
        m_lineResources.uniformBuffer)});
    m_lineResources.shaderBindings->create();

    // Sprite resources
    m_spriteResources.vertexBuffer =
        rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, 6 * 2 * sizeof(float));
    m_spriteResources.vertexBuffer->create();

    m_spriteResources.uniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                                                     UNIFORM_ALIGNMENT * MAX_MARKERS);
    m_spriteResources.uniformBuffer->create();

    m_spriteResources.shaderBindings = rhi->newShaderResourceBindings();
    m_spriteResources.shaderBindings->setBindings(
        {QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_spriteResources.uniformBuffer, sizeof(SpriteUniformData))});
    m_spriteResources.shaderBindings->create();

    createLinePipeline(rhi);
    createSpritePipeline(rhi);
    m_initialized = true;
}

void MarkerRenderer::createLinePipeline(QRhi *rhi) {
    delete m_lineResources.pipeline;
    m_lineResources.pipeline = rhi->newGraphicsPipeline();

    // Reuse pin shaders for lines (same format: position + color)
    QFile vs(":/shaders/shaders/pin.vert.qsb");
    QFile fs(":/shaders/shaders/pin.frag.qsb");
    QShader vertShader, fragShader;
    if (vs.open(QIODevice::ReadOnly)) {
        vertShader = QShader::fromSerialized(vs.readAll());
        vs.close();
    }
    if (fs.open(QIODevice::ReadOnly)) {
        fragShader = QShader::fromSerialized(fs.readAll());
        fs.close();
    }
    if (!vertShader.isValid() || !fragShader.isValid())
        return;

    m_lineResources.pipeline->setShaderStages(
        {{QRhiShaderStage::Vertex, vertShader}, {QRhiShaderStage::Fragment, fragShader}});

    QRhiVertexInputLayout layout;
    layout.setBindings({{sizeof(LineVertex)}});
    layout.setAttributes({{0, 0, QRhiVertexInputAttribute::Float3, offsetof(LineVertex, position)},
                          {0, 1, QRhiVertexInputAttribute::Float3, offsetof(LineVertex, color)}});
    m_lineResources.pipeline->setVertexInputLayout(layout);
    m_lineResources.pipeline->setShaderResourceBindings(m_lineResources.shaderBindings);

    QRhiRenderTarget *rt = renderTarget();
    if (rt)
        m_lineResources.pipeline->setRenderPassDescriptor(rt->renderPassDescriptor());

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_lineResources.pipeline->setTargetBlends({blend});

    m_lineResources.pipeline->setDepthTest(true);
    m_lineResources.pipeline->setDepthWrite(false);
    m_lineResources.pipeline->setTopology(QRhiGraphicsPipeline::LineStrip);

    m_lineResources.pipeline->create();
}

void MarkerRenderer::createSpritePipeline(QRhi *rhi) {
    delete m_spriteResources.pipeline;
    m_spriteResources.pipeline = rhi->newGraphicsPipeline();

    // Reuse pintop shaders
    QFile vs(":/shaders/shaders/pintop.vert.qsb");
    QFile fs(":/shaders/shaders/pintop.frag.qsb");
    QShader vertShader, fragShader;
    if (vs.open(QIODevice::ReadOnly)) {
        vertShader = QShader::fromSerialized(vs.readAll());
        vs.close();
    }
    if (fs.open(QIODevice::ReadOnly)) {
        fragShader = QShader::fromSerialized(fs.readAll());
        fs.close();
    }
    if (!vertShader.isValid() || !fragShader.isValid())
        return;

    m_spriteResources.pipeline->setShaderStages(
        {{QRhiShaderStage::Vertex, vertShader}, {QRhiShaderStage::Fragment, fragShader}});

    QRhiVertexInputLayout layout;
    layout.setBindings({{2 * sizeof(float)}});
    layout.setAttributes({{0, 0, QRhiVertexInputAttribute::Float2, 0}});
    m_spriteResources.pipeline->setVertexInputLayout(layout);
    m_spriteResources.pipeline->setShaderResourceBindings(m_spriteResources.shaderBindings);

    QRhiRenderTarget *rt = renderTarget();
    if (rt)
        m_spriteResources.pipeline->setRenderPassDescriptor(rt->renderPassDescriptor());

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_spriteResources.pipeline->setTargetBlends({blend});

    m_spriteResources.pipeline->setDepthTest(true);
    m_spriteResources.pipeline->setDepthWrite(false);

    m_spriteResources.pipeline->create();
}

void MarkerRenderer::setMarkers(const std::vector<MarkerData> &markers) {
    m_markers = markers;
    m_geometryDirty = true;
    m_needsPipelineRebuild = false;
}
