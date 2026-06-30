#include <QCoreApplication>
#include <QApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QQmlContext>
#include <QQuickStyle>
#include <QDebug>

#include "app/single_instance_guard.h"
#include "app/tray_service.h"
#include "ui_bridge/application_controller.h"
#include "player/playback_service.h"
#include "settings/settings_service.h"
#include "i18n/ui_translation_service.h"

#ifdef Q_OS_WIN
#include "app/windows_app_identity.h"
#endif

#ifdef Q_OS_LINUX
#include <QFile>
#include <QFileInfo>

namespace {

bool prependQtPluginPath(const QString &pluginsRoot)
{
    if (pluginsRoot.isEmpty() || !QFileInfo::exists(pluginsRoot)) {
        return false;
    }
    const QByteArray pathBytes = pluginsRoot.toUtf8();
    const QByteArray existing = qgetenv("QT_PLUGIN_PATH");
    if (existing.isEmpty()) {
        qputenv("QT_PLUGIN_PATH", pathBytes);
    } else if (!existing.split(':').contains(pathBytes)) {
        qputenv("QT_PLUGIN_PATH", pathBytes + ':' + existing);
    }
    return true;
}

bool hasFcitx5PluginInDir(const QString &platformInputContextsDir)
{
    return QFile::exists(platformInputContextsDir
                         + QStringLiteral("/libfcitx5platforminputcontextplugin.so"));
}

void setupLinuxInputMethod(const char *argv0)
{
    // apt 自带的 fcitx5 Qt 插件链接系统 Qt 6.4，与 Qt 在线安装器版本 ABI 不兼容。
    // 仅加载与当前 Qt 同版本编译的插件（见 scripts/build-fcitx5-qt-plugin.sh）。
    const QString appDir = QFileInfo(QString::fromLocal8Bit(argv0)).absolutePath();
    const QString qtDir = QString::fromLocal8Bit(qgetenv("QT_DIR"));

    const QStringList fcitxPluginDirs = {
        appDir + QStringLiteral("/platforminputcontexts"),
        qtDir + QStringLiteral("/plugins/platforminputcontexts"),
    };

    for (const QString &dir : fcitxPluginDirs) {
        if (!hasFcitx5PluginInDir(dir)) {
            continue;
        }
        if (prependQtPluginPath(QFileInfo(dir).absolutePath())) {
            qputenv("QT_IM_MODULE", QByteArray("fcitx"));
            qInfo().noquote() << QStringLiteral("已启用 fcitx5 输入法插件：%1").arg(dir);
            return;
        }
    }

    qWarning().noquote()
        << QStringLiteral(
               "未找到与当前 Qt 版本匹配的 fcitx5 插件，文本框可能无法输入中文。"
               "请运行 scripts/build-fcitx5-qt-plugin.sh 构建并安装插件。");
}

} // namespace
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    setupLinuxInputMethod(argv[0]);
#endif

#ifdef Q_OS_WIN
    QGuiApplication::setDesktopFileName(QStringLiteral("Xiaoxiong.XiaoxiongMusicPlayer"));
#endif

    QApplication app(argc, argv);
    QGuiApplication::setQuitOnLastWindowClosed(false);

#ifdef Q_OS_WIN
    WindowsAppIdentity::initializeProcessIdentity();
    WindowsAppIdentity::ensureShellRegistration();
#endif

    QCoreApplication::setOrganizationName(QStringLiteral("Xiaoxiong"));
    QCoreApplication::setApplicationName(QStringLiteral("XiaoxiongMusicPlayer"));

    SingleInstanceGuard singleInstanceGuard;
    if (!singleInstanceGuard.tryAcquirePrimaryInstance()) {
        return 0;
    }

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

    QObject::connect(
        &singleInstanceGuard, &SingleInstanceGuard::activationRequested,
        controller.trayService(), [trayService = qobject_cast<TrayService *>(controller.trayService())]() {
            if (trayService) {
                trayService->showMainWindow();
            }
        },
        Qt::QueuedConnection);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, &controller, [&controller]() {
        if (PlaybackService *playback = qobject_cast<PlaybackService *>(controller.playbackService())) {
            playback->saveSession();
        }
    });

    qDebug() << "Application started";
    return app.exec();
}
