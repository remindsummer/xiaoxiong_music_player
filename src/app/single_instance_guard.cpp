#include "single_instance_guard.h"

#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDebug>

namespace {

constexpr char kActivateMessage[] = "activate";

} // namespace

SingleInstanceGuard::SingleInstanceGuard(QObject *parent)
    : QObject(parent)
{
}

SingleInstanceGuard::~SingleInstanceGuard()
{
    if (m_server) {
        m_server->close();
    }
}

QString SingleInstanceGuard::serverKey()
{
    // 勿用 '/'：Linux 下 QLocalServer 会将其当作路径分隔符，/tmp/Xiaoxiong/ 不存在时 listen 失败。
    return QCoreApplication::organizationName() + QLatin1Char('.')
           + QCoreApplication::applicationName();
}

bool SingleInstanceGuard::notifyPrimaryInstanceAndExit()
{
    QLocalSocket socket;
    socket.connectToServer(serverKey());
    if (!socket.waitForConnected(500)) {
        return false;
    }

    socket.write(kActivateMessage);
    socket.waitForBytesWritten(500);
    socket.disconnectFromServer();
    if (socket.state() != QLocalSocket::UnconnectedState) {
        socket.waitForDisconnected(500);
    }
    return true;
}

bool SingleInstanceGuard::tryAcquirePrimaryInstance()
{
    if (notifyPrimaryInstanceAndExit()) {
        qInfo().noquote() << QStringLiteral("已有实例在运行，激活主窗口后退出。");
        return false;
    }

    const QString key = serverKey();
    QLocalServer::removeServer(key);

    m_server = new QLocalServer(this);
    connect(m_server, &QLocalServer::newConnection, this, &SingleInstanceGuard::handleNewConnection);

    if (!m_server->listen(key)) {
        qWarning().noquote() << QStringLiteral("单实例锁 listen 失败：%1（key=%2）")
                                    .arg(m_server->errorString(), key);
        if (notifyPrimaryInstanceAndExit()) {
            return false;
        }
        return false;
    }

    m_isPrimary = true;
    return true;
}

void SingleInstanceGuard::handleNewConnection()
{
    if (!m_server) {
        return;
    }

    QLocalSocket *socket = m_server->nextPendingConnection();
    if (!socket) {
        return;
    }

    connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
        const QByteArray message = socket->readAll();
        if (message == kActivateMessage) {
            emit activationRequested();
        }
        socket->disconnectFromServer();
    });
    connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
}
