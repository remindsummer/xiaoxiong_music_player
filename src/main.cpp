#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QQmlContext>
#include <QQuickStyle>
#include <QDebug>

#include "ui_bridge/application_controller.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // 设置组织/应用名，使 QStandardPaths（settings.json、曲库/歌单持久化）
    // 落到稳定且唯一的本地目录。
    QCoreApplication::setOrganizationName(QStringLiteral("Xiaoxiong"));
    QCoreApplication::setApplicationName(QStringLiteral("XiaoxiongMusicPlayer"));

    // 本项目大量使用 Button 的 background/contentItem 自定义，
    // 而 Windows 原生样式不支持这些自定义。改用 Basic 样式使其生效。
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QQmlApplicationEngine engine;
    ApplicationController controller;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &app, [](const QList<QQmlError> &warnings) {
        for (const QQmlError &warning : warnings) {
            qWarning().noquote() << warning.toString();
        }
    });

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/XiaoxiongMusic/qml/Main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    qDebug() << "Application started";
    return app.exec();
}
