#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "GameManager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    GameManager gameManager;
    engine.rootContext()->setContextProperty("gameManager", &gameManager);
    engine.rootContext()->setContextProperty("obstacleModel", gameManager.obstacleModel());

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("DinoRun", "Main");

    return QGuiApplication::exec();
}
