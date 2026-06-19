#pragma once

#include <QObject>
#include <QSystemTrayIcon>
class QAction;
class QMenu;
class QWindow;

class TrayService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)

public:
    explicit TrayService(QObject *parent = nullptr);
    ~TrayService() override;

    bool isAvailable() const;

    Q_INVOKABLE bool initialize();
    Q_INVOKABLE void setWindow(QObject *windowObject);
    Q_INVOKABLE void showMainWindow();
    Q_INVOKABLE void hideMainWindow();
    Q_INVOKABLE void quitApplication();

signals:
    void availableChanged();

private:
    void handleActivated(QSystemTrayIcon::ActivationReason reason);
    void rebuildTrayTexts();

    QWindow *m_window = nullptr;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;
    QAction *m_openAction = nullptr;
    QAction *m_quitAction = nullptr;
    bool m_available = false;
};
