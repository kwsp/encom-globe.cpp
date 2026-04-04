#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "config.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.loadFromModule(APP_MODULE_NAME, "Main");

    return QGuiApplication::exec();
}
