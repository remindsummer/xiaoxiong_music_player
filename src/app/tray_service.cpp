#include "tray_service.h"

#include <QAction>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <QWindow>

#include <QSvgRenderer>

namespace {

QIcon iconFromSvgResource(const QString &resourcePath)
{
    QSvgRenderer renderer(resourcePath);
    if (!renderer.isValid()) {
        return QIcon();
    }

    QIcon icon;
    const QList<int> sizes = {16, 20, 24, 32, 48, 64};
    for (const int size : sizes) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        renderer.render(&painter, QRectF(0, 0, size, size));
        painter.end();
        icon.addPixmap(pixmap);
    }
    return icon;
}

void drawMusicNoteGlyph(QPainter &painter, int size)
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);

    const qreal s = static_cast<qreal>(size);
    painter.drawRect(QRectF(s * 0.44, s * 0.28, s * 0.08, s * 0.42));
    painter.drawEllipse(QRectF(s * 0.32, s * 0.58, s * 0.24, s * 0.18));
}

QIcon createFallbackTrayIcon()
{
    QIcon icon;
    const QList<int> sizes = {16, 20, 24, 32, 48, 64};
    for (const int size : sizes) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const qreal inset = size * 0.0625;
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0x3B, 0x82, 0xF6));
        painter.drawEllipse(QRectF(inset, inset, size - inset * 2, size - inset * 2));

        drawMusicNoteGlyph(painter, size);
        painter.end();

        icon.addPixmap(pixmap);
    }
    return icon;
}

QIcon loadTrayIcon()
{
    const QStringList candidates = {
        QStringLiteral(":/XiaoxiongMusic/assets/icons/tray_icon.svg"),
        QStringLiteral(":/XiaoxiongMusic/assets/icons/music_note.svg"),
    };

    for (const QString &path : candidates) {
        const QIcon icon = iconFromSvgResource(path);
        if (!icon.isNull()) {
            return icon;
        }
    }

    const QIcon themedIcon = QIcon::fromTheme(QStringLiteral("audio-x-generic"));
    if (!themedIcon.isNull()) {
        return themedIcon;
    }

    return createFallbackTrayIcon();
}

} // namespace

TrayService::TrayService(QObject *parent)
    : QObject(parent)
{
}

TrayService::~TrayService()
{
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
}

bool TrayService::isAvailable() const
{
    return m_available;
}

bool TrayService::initialize()
{
    const bool systemTrayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    if (!systemTrayAvailable) {
        if (m_available) {
            m_available = false;
            emit availableChanged();
        }
        return false;
    }

    if (m_trayIcon) {
        if (!m_available) {
            m_available = true;
            emit availableChanged();
        }
        return true;
    }

    m_trayMenu = new QMenu();
    m_openAction = m_trayMenu->addAction(QString(), this, &TrayService::showMainWindow);
    m_quitAction = m_trayMenu->addAction(QString(), this, &TrayService::quitApplication);
    rebuildTrayTexts();

    const QIcon trayIcon = loadTrayIcon();
    m_trayIcon = new QSystemTrayIcon(trayIcon, this);
    m_trayIcon->setToolTip(tr("小熊音乐播放器"));
    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayService::handleActivated);
    m_trayIcon->show();

    if (!m_available) {
        m_available = true;
        emit availableChanged();
    }
    return true;
}

void TrayService::setWindow(QObject *windowObject)
{
    m_window = qobject_cast<QWindow *>(windowObject);
}

void TrayService::showMainWindow()
{
    if (!m_window) {
        return;
    }

    m_window->show();
    m_window->raise();
    m_window->requestActivate();
}

void TrayService::hideMainWindow()
{
    if (m_window) {
        m_window->hide();
    }
    if (m_trayIcon) {
        m_trayIcon->show();
    }
}

void TrayService::quitApplication()
{
    QCoreApplication::quit();
}

void TrayService::handleActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        showMainWindow();
    }
}

void TrayService::rebuildTrayTexts()
{
    if (m_openAction) {
        m_openAction->setText(tr("打开主界面"));
    }
    if (m_quitAction) {
        m_quitAction->setText(tr("退出"));
    }
    if (m_trayIcon) {
        m_trayIcon->setToolTip(tr("小熊音乐播放器"));
    }
}
