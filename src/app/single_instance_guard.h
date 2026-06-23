#pragma once

#include <QObject>

class QLocalServer;

class SingleInstanceGuard : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstanceGuard(QObject *parent = nullptr);
    ~SingleInstanceGuard() override;

    bool tryAcquirePrimaryInstance();
    static bool notifyPrimaryInstanceAndExit();

signals:
    void activationRequested();

private:
    void handleNewConnection();

    static QString serverKey();

    QLocalServer *m_server = nullptr;
    bool m_isPrimary = false;
};
