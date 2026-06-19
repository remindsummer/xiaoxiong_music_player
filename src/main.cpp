#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QQmlContext>
#include <QQuickStyle>
#include <QDebug>

#include "ui_bridge/application_controller.h"
#include "player/playback_service.h"
#include "settings/settings_service.h"
#include "i18n/ui_translation_service.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("Xiaoxiong"));
    QCoreApplication::setApplicationName(QStringLiteral("XiaoxiongMusicPlayer"));

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QQmlApplicationEngine engine;
    ApplicationController controller;
    UiTranslationService translationService(&app, &engine);
    controller.setUiTranslationService(&translationService);

    if (SettingsService *settings = qobject_cast<SettingsService *>(controller.settingsService())) {
        translationService.applyLanguage(settings->language());
    }

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

    QObject::connect(&app, &QGuiApplication::aboutToQuit, &controller, [&controller]() {
        if (PlaybackService *playback = qobject_cast<PlaybackService *>(controller.playbackService())) {
            playback->saveSession();
        }
    });

    qDebug() << "Application started";
    return app.exec();
}
