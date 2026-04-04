#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSurfaceFormat>

#include "config.h"

int main(int argc, char *argv[]) {
    // Request high DPI scaling before creating QGuiApplication
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    
    QGuiApplication app(argc, argv);
    
    QQmlApplicationEngine engine;
    
    // Load the main QML file from the module
    engine.loadFromModule(APP_MODULE_NAME, "Main");
    
    if (engine.rootObjects().isEmpty()) {
        qWarning() << "Failed to load QML from module" << APP_MODULE_NAME;
        return -1;
    }
    
    return QGuiApplication::exec();
}
