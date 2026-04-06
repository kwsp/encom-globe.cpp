#include <QFile>
#include <rhi/qrhi.h>

struct RhiResources {
    // RHI resources
    QRhiGraphicsPipeline *pipeline{};
    QRhiBuffer *vertexBuffer{};
    QRhiBuffer *uniformBuffer{};
    QRhiShaderResourceBindings *shaderBindings{};

    void releaseResources() {
        delete pipeline;
        pipeline = nullptr;

        delete vertexBuffer;
        vertexBuffer = nullptr;

        delete uniformBuffer;
        uniformBuffer = nullptr;

        delete shaderBindings;
        shaderBindings = nullptr;
    }
};

inline std::tuple<QShader, QShader> loadShaders(const QString &vsPath, const QString &fsPath) {
    QFile vsFile(vsPath);
    QFile fsFile(fsPath);
    QShader vertexShader;
    QShader fragmentShader;

    if (vsFile.open(QIODevice::ReadOnly)) {
        vertexShader = QShader::fromSerialized(vsFile.readAll());
        vsFile.close();
    } else {
        qWarning() << "Failed to load vertex shader:" << vsFile.errorString();
        return {};
    }

    if (fsFile.open(QIODevice::ReadOnly)) {
        fragmentShader = QShader::fromSerialized(fsFile.readAll());
        fsFile.close();
    } else {
        qWarning() << "Failed to load fragment shader:" << fsFile.errorString();
        return {};
    }

    if (!vertexShader.isValid() || !fragmentShader.isValid()) {
        qWarning() << "Shaders are not valid:" << vsPath << fsPath;
        return {};
    }

    return {vertexShader, fragmentShader};
}