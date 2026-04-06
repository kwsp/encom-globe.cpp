import os, re

def replace_in_file(path, replacements):
    with open(path, 'r') as f:
        content = f.read()
    for old, new in replacements:
        content = content.replace(old, new)
    with open(path, 'w') as f:
        f.write(content)

# PinRenderer
replace_in_file('lib/PinRenderer.h', [
    ('#include <vector>', '#include <vector>\n#include "RhiResources.h"'),
    ('    QRhiGraphicsPipeline *m_pipeline = nullptr;\n    QRhiBuffer *m_vertexBuffer = nullptr;\n    QRhiBuffer *m_uniformBuffer = nullptr;\n    QRhiShaderResourceBindings *m_shaderBindings = nullptr;\n\n    // Top resources\n    QRhiGraphicsPipeline *m_topPipeline = nullptr;\n    QRhiBuffer *m_topVertexBuffer = nullptr;\n    QRhiBuffer *m_topUniformBuffer = nullptr;\n    QRhiShaderResourceBindings *m_topShaderBindings = nullptr;', '    RhiResources m_lineResources;\n    RhiResources m_topResources;'),
])

pin_delete_old = """    delete m_pipeline;
    m_pipeline = nullptr;
    delete m_vertexBuffer;
    m_vertexBuffer = nullptr;
    delete m_uniformBuffer;
    m_uniformBuffer = nullptr;
    delete m_shaderBindings;
    m_shaderBindings = nullptr;

    delete m_topPipeline;
    m_topPipeline = nullptr;
    delete m_topVertexBuffer;
    m_topVertexBuffer = nullptr;
    delete m_topUniformBuffer;
    m_topUniformBuffer = nullptr;
    delete m_topShaderBindings;
    m_topShaderBindings = nullptr;"""

pin_line_shader_old = """    QShader vertexShader;
    QShader fragmentShader;
    QFile vsFile(":/shaders/shaders/pin.vert.qsb");
    QFile fsFile(":/shaders/shaders/pin.frag.qsb");

    if (vsFile.open(QIODevice::ReadOnly)) {
        vertexShader = QShader::fromSerialized(vsFile.readAll());
        vsFile.close();
    } else {
        qWarning() << "Failed to load pin vertex shader:" << vsFile.errorString();
    }

    if (fsFile.open(QIODevice::ReadOnly)) {
        fragmentShader = QShader::fromSerialized(fsFile.readAll());
        fsFile.close();
    } else {
        qWarning() << "Failed to load pin fragment shader:" << fsFile.errorString();
    }"""

pin_top_shader_old = """    QShader topVertexShader;
    QShader topFragmentShader;
    QFile topVsFile(":/shaders/shaders/pintop.vert.qsb");
    QFile topFsFile(":/shaders/shaders/pintop.frag.qsb");

    if (topVsFile.open(QIODevice::ReadOnly)) {
        topVertexShader = QShader::fromSerialized(topVsFile.readAll());
        topVsFile.close();
    } else {
        qWarning() << "Failed to load pintop vertex shader:" << topVsFile.errorString();
    }

    if (topFsFile.open(QIODevice::ReadOnly)) {
        topFragmentShader = QShader::fromSerialized(topFsFile.readAll());
        topFsFile.close();
    } else {
        qWarning() << "Failed to load pintop fragment shader:" << topFsFile.errorString();
    }"""

replace_in_file('lib/PinRenderer.cpp', [
    (pin_delete_old, '    m_lineResources.releaseResources();\n    m_topResources.releaseResources();'),
    (pin_line_shader_old, '    auto [vertexShader, fragmentShader] = RhiResources::loadShaders(":/shaders/shaders/pin.vert.qsb", ":/shaders/shaders/pin.frag.qsb");'),
    (pin_top_shader_old, '    auto [topVertexShader, topFragmentShader] = RhiResources::loadShaders(":/shaders/shaders/pintop.vert.qsb", ":/shaders/shaders/pintop.frag.qsb");'),
    ('m_pipeline', 'm_lineResources.pipeline'),
    ('m_vertexBuffer', 'm_lineResources.vertexBuffer'),
    ('m_uniformBuffer', 'm_lineResources.uniformBuffer'),
    ('m_shaderBindings', 'm_lineResources.shaderBindings'),
    ('m_topPipeline', 'm_topResources.pipeline'),
    ('m_topVertexBuffer', 'm_topResources.vertexBuffer'),
    ('m_topUniformBuffer', 'm_topResources.uniformBuffer'),
    ('m_topShaderBindings', 'm_topResources.shaderBindings'),
])

# MarkerRenderer
replace_in_file('lib/MarkerRenderer.h', [
    ('#include <vector>', '#include <vector>\n#include "RhiResources.h"'),
    ('    QRhiGraphicsPipeline *m_linePipeline = nullptr;\n    QRhiBuffer *m_lineVertexBuffer = nullptr;\n    QRhiBuffer *m_lineUniformBuffer = nullptr;\n    QRhiShaderResourceBindings *m_lineBindings = nullptr;\n\n    // Sprite resources\n    QRhiGraphicsPipeline *m_spritePipeline = nullptr;\n    QRhiBuffer *m_spriteVertexBuffer = nullptr;\n    QRhiBuffer *m_spriteUniformBuffer = nullptr;\n    QRhiShaderResourceBindings *m_spriteBindings = nullptr;', '    RhiResources m_lineResources;\n    RhiResources m_spriteResources;'),
])

marker_delete_old = """    delete m_linePipeline;
    m_linePipeline = nullptr;
    delete m_lineVertexBuffer;
    m_lineVertexBuffer = nullptr;
    delete m_lineUniformBuffer;
    m_lineUniformBuffer = nullptr;
    delete m_lineBindings;
    m_lineBindings = nullptr;

    delete m_spritePipeline;
    m_spritePipeline = nullptr;
    delete m_spriteVertexBuffer;
    m_spriteVertexBuffer = nullptr;
    delete m_spriteUniformBuffer;
    m_spriteUniformBuffer = nullptr;
    delete m_spriteBindings;
    m_spriteBindings = nullptr;"""

marker_line_shader_old = """    QShader vertShader, fragShader;
    QFile vs(":/shaders/shaders/line.vert.qsb");
    QFile fs(":/shaders/shaders/line.frag.qsb");
    if (vs.open(QIODevice::ReadOnly))
        vertShader = QShader::fromSerialized(vs.readAll());
    if (fs.open(QIODevice::ReadOnly))
        fragShader = QShader::fromSerialized(fs.readAll());"""

marker_sprite_shader_old = """    QShader vertShader, fragShader;
    QFile vs(":/shaders/shaders/satellite.vert.qsb");
    QFile fs(":/shaders/shaders/satellite.frag.qsb");
    if (vs.open(QIODevice::ReadOnly))
        vertShader = QShader::fromSerialized(vs.readAll());
    if (fs.open(QIODevice::ReadOnly))
        fragShader = QShader::fromSerialized(fs.readAll());"""

replace_in_file('lib/MarkerRenderer.cpp', [
    (marker_delete_old, '    m_lineResources.releaseResources();\n    m_spriteResources.releaseResources();'),
    (marker_line_shader_old, '    auto [vertShader, fragShader] = RhiResources::loadShaders(":/shaders/shaders/line.vert.qsb", ":/shaders/shaders/line.frag.qsb");'),
    (marker_sprite_shader_old, '    auto [vertShader, fragShader] = RhiResources::loadShaders(":/shaders/shaders/satellite.vert.qsb", ":/shaders/shaders/satellite.frag.qsb");'),
    ('m_linePipeline', 'm_lineResources.pipeline'),
    ('m_lineVertexBuffer', 'm_lineResources.vertexBuffer'),
    ('m_lineUniformBuffer', 'm_lineResources.uniformBuffer'),
    ('m_lineBindings', 'm_lineResources.shaderBindings'),
    ('m_spritePipeline', 'm_spriteResources.pipeline'),
    ('m_spriteVertexBuffer', 'm_spriteResources.vertexBuffer'),
    ('m_spriteUniformBuffer', 'm_spriteResources.uniformBuffer'),
    ('m_spriteBindings', 'm_spriteResources.shaderBindings'),
])

