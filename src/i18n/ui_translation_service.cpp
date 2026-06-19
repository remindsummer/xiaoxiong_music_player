#include "ui_translation_service.h"

#include <QGuiApplication>
#include <QLocale>
#include <QQmlApplicationEngine>

namespace {
const char *kTranslationResourcePrefix = ":/i18n";
const char *kTranslationBaseName = "xiaoxiong";
} // namespace

UiTranslationService::UiTranslationService(QGuiApplication *app,
                                           QQmlApplicationEngine *engine,
                                           QObject *parent)
    : QObject(parent)
    , m_app(app)
    , m_engine(engine)
{
}

QString UiTranslationService::currentLanguage() const
{
    return m_currentLanguage;
}

QString UiTranslationService::normalizeLanguageCode(const QString &languageCode) const
{
    const QString trimmed = languageCode.trimmed();
    if (trimmed == QStringLiteral("en_US") || trimmed == QStringLiteral("en")) {
        return QStringLiteral("en_US");
    }
    return QStringLiteral("zh_CN");
}

bool UiTranslationService::applyLanguage(const QString &languageCode)
{
    if (!m_app || !m_engine) {
        return false;
    }

    const QString normalized = normalizeLanguageCode(languageCode);

    m_app->removeTranslator(&m_translator);
    m_translatorInstalled = false;

    const QString qmName = QStringLiteral("%1_%2").arg(QString::fromLatin1(kTranslationBaseName), normalized);
    if (m_translator.load(qmName, QString::fromLatin1(kTranslationResourcePrefix))) {
        m_app->installTranslator(&m_translator);
        m_translatorInstalled = true;
    }

    m_currentLanguage = normalized;
    m_engine->retranslate();
    emit languageApplied(normalized);
    return true;
}
