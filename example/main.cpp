#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QUrl>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(EncomGlobePlugin)

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    
    QQmlApplicationEngine engine;

    QAnyStringView moduleUri = "EncomGlobeExample";
    engine.loadFromModule(moduleUri, "Main");
    
    if (engine.rootObjects().isEmpty()) {
        qWarning() << "Failed to load QML from module" << moduleUri;
        return -1;
    }
    
    return QGuiApplication::exec();
}
