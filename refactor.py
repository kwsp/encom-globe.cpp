import os, re

def replace_in_file(path, replacements):
    with open(path, 'r') as f:
        content = f.read()
    for old, new in replacements:
        content = content.replace(old, new)
    with open(path, 'w') as f:
        f.write(content)

# GlobeRenderer
replace_in_file('lib/GlobeRenderer.h', [
    ('#include <vector>', '#include <vector>\n#include "RhiResources.h"'),
    ('    QRhiGraphicsPipeline *m_pipeline{};\n    QRhiBuffer *m_vertexBuffer{};\n    QRhiBuffer *m_uniformBuffer{};\n    QRhiShaderResourceBindings *m_shaderBindings{};', '    RhiResources m_rhiResources;'),
])

globe_shader_old = """    QShader vertexShader;
    QShader fragmentShader;
    QFile vsFile(":/shaders/shaders/globe.vert.qsb");
    QFile fsFile(":/shaders/shaders/globe.frag.qsb");

    if (vsFile.open(QIODevice::ReadOnly)) {
        vertexShader = QShader::fromSerialized(vsFile.readAll());
        vsFile.close();
    } else {
        qWarning() << "Failed to load vertex shader:" << vsFile.errorString();
    }

    if (fsFile.open(QIODevice::ReadOnly)) {
        fragmentShader = QShader::fromSerialized(fsFile.readAll());
        fsFile.close();
    } else {
        qWarning() << "Failed to load fragment shader:" << fsFile.errorString();
    }"""

globe_delete_old = """    delete m_pipeline;
    m_pipeline = nullptr;

    delete m_vertexBuffer;
    m_vertexBuffer = nullptr;

    delete m_uniformBuffer;
    m_uniformBuffer = nullptr;

    delete m_shaderBindings;
    m_shaderBindings = nullptr;"""

replace_in_file('lib/GlobeRenderer.cpp', [
    (globe_delete_old, '    m_rhiResources.releaseResources();'),
    (globe_shader_old, '    auto [vertexShader, fragmentShader] = RhiResources::loadShaders(":/shaders/shaders/globe.vert.qsb", ":/shaders/shaders/globe.frag.qsb");'),
    ('m_pipeline', 'm_rhiResources.pipeline'),
    ('m_vertexBuffer', 'm_rhiResources.vertexBuffer'),
    ('m_uniformBuffer', 'm_rhiResources.uniformBuffer'),
    ('m_shaderBindings', 'm_rhiResources.shaderBindings'),
])

# IntroLinesRenderer
replace_in_file('lib/IntroLinesRenderer.h', [
    ('#include <vector>', '#include <vector>\n#include "RhiResources.h"'),
    ('    QRhiGraphicsPipeline *m_pipeline{};\n    QRhiBuffer *m_vertexBuffer{};\n    QRhiBuffer *m_uniformBuffer{};\n    QRhiShaderResourceBindings *m_bindings{};', '    RhiResources m_rhiResources;'),
])

intro_delete_old = """    delete m_pipeline;
    m_pipeline = nullptr;
    delete m_vertexBuffer;
    m_vertexBuffer = nullptr;
    delete m_uniformBuffer;
    m_uniformBuffer = nullptr;
    delete m_bindings;
    m_bindings = nullptr;"""

intro_shader_old = """    QShader v, f;
    QFile vs(":/shaders/shaders/introlines.vert.qsb");
    QFile fs(":/shaders/shaders/introlines.frag.qsb");
    if (vs.open(QIODevice::ReadOnly))
        v = QShader::fromSerialized(vs.readAll());
    if (fs.open(QIODevice::ReadOnly))
        f = QShader::fromSerialized(fs.readAll());"""

replace_in_file('lib/IntroLinesRenderer.cpp', [
    (intro_delete_old, '    m_rhiResources.releaseResources();'),
    (intro_shader_old, '    auto [v, f] = RhiResources::loadShaders(":/shaders/shaders/introlines.vert.qsb", ":/shaders/shaders/introlines.frag.qsb");'),
    ('m_pipeline', 'm_rhiResources.pipeline'),
    ('m_vertexBuffer', 'm_rhiResources.vertexBuffer'),
    ('m_uniformBuffer', 'm_rhiResources.uniformBuffer'),
    ('m_bindings', 'm_rhiResources.shaderBindings'),
])

# SmokeRenderer
replace_in_file('lib/SmokeRenderer.h', [
    ('#include <vector>', '#include <vector>\n#include "RhiResources.h"'),
    ('    QRhiGraphicsPipeline *m_pipeline{};\n    QRhiBuffer *m_vertexBuffer{};\n    QRhiBuffer *m_uniformBuffer{};\n    QRhiShaderResourceBindings *m_bindings{};', '    RhiResources m_rhiResources;'),
])

smoke_delete_old = """    delete m_pipeline;
    m_pipeline = nullptr;
    delete m_vertexBuffer;
    m_vertexBuffer = nullptr;
    delete m_uniformBuffer;
    m_uniformBuffer = nullptr;
    delete m_bindings;
    m_bindings = nullptr;"""

smoke_shader_old = """    QShader v, f;
    QFile vs(":/shaders/shaders/smoke.vert.qsb");
    QFile fs(":/shaders/shaders/smoke.frag.qsb");
    if (vs.open(QIODevice::ReadOnly))
        v = QShader::fromSerialized(vs.readAll());
    if (fs.open(QIODevice::ReadOnly))
        f = QShader::fromSerialized(fs.readAll());"""

replace_in_file('lib/SmokeRenderer.cpp', [
    (smoke_delete_old, '    m_rhiResources.releaseResources();'),
    (smoke_shader_old, '    auto [v, f] = RhiResources::loadShaders(":/shaders/shaders/smoke.vert.qsb", ":/shaders/shaders/smoke.frag.qsb");'),
    ('m_pipeline', 'm_rhiResources.pipeline'),
    ('m_vertexBuffer', 'm_rhiResources.vertexBuffer'),
    ('m_uniformBuffer', 'm_rhiResources.uniformBuffer'),
    ('m_bindings', 'm_rhiResources.shaderBindings'),
])
