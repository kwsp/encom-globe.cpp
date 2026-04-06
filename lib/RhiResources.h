#include <QDebug>
#include <QFile>
#include <rhi/qrhi.h>
#include <rhi/qshader.h>
#include <tuple>

struct RhiResources {
    // RHI resources
    QRhiGraphicsPipeline *pipeline{};
    QRhiBuffer *vertexBuffer{};
    QRhiBuffer *uniformBuffer{};
    QRhiShaderResourceBindings *shaderBindings{};

    RhiResources() = default;

    // No copy
    RhiResources(const RhiResources &) = delete;
    RhiResources &operator=(const RhiResources &) = delete;

    // Safe move
    RhiResources(RhiResources &&other) noexcept
        : pipeline(other.pipeline), vertexBuffer(other.vertexBuffer),
          uniformBuffer(other.uniformBuffer), shaderBindings(other.shaderBindings) {
        other.pipeline = nullptr;
        other.vertexBuffer = nullptr;
        other.uniformBuffer = nullptr;
        other.shaderBindings = nullptr;
    }

    RhiResources &operator=(RhiResources &&other) noexcept {
        if (this != &other) {
            releaseResources();
            pipeline = other.pipeline;
            vertexBuffer = other.vertexBuffer;
            uniformBuffer = other.uniformBuffer;
            shaderBindings = other.shaderBindings;

            other.pipeline = nullptr;
            other.vertexBuffer = nullptr;
            other.uniformBuffer = nullptr;
            other.shaderBindings = nullptr;
        }
        return *this;
    }

    ~RhiResources() { releaseResources(); }

    void releaseResources() {
        if (pipeline) {
            delete pipeline;
            pipeline = nullptr;
        }

        if (vertexBuffer) {
            delete vertexBuffer;
            vertexBuffer = nullptr;
        }

        if (uniformBuffer) {
            delete uniformBuffer;
            uniformBuffer = nullptr;
        }

        if (shaderBindings) {
            delete shaderBindings;
            shaderBindings = nullptr;
        }
    }

    static std::tuple<QShader, QShader> loadShaders(const QString &vsPath, const QString &fsPath) {
        QFile vsFile(vsPath);
        QFile fsFile(fsPath);
        QShader vertexShader;
        QShader fragmentShader;

        if (vsFile.open(QIODevice::ReadOnly)) {
            vertexShader = QShader::fromSerialized(vsFile.readAll());
            vsFile.close();
        } else {
            qWarning() << "Failed to load vertex shader:" << vsFile.errorString() << vsPath;
            return {};
        }

        if (fsFile.open(QIODevice::ReadOnly)) {
            fragmentShader = QShader::fromSerialized(fsFile.readAll());
            fsFile.close();
        } else {
            qWarning() << "Failed to load fragment shader:" << fsFile.errorString() << fsPath;
            return {};
        }

        if (!vertexShader.isValid() || !fragmentShader.isValid()) {
            qWarning() << "Shaders are not valid:" << vsPath << fsPath;
            return {};
        }

        return {vertexShader, fragmentShader};
    }
};