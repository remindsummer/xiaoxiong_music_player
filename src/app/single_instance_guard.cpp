#include "single_instance_guard.h"

#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>

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
    return QCoreApplication::organizationName() + QLatin1Char('/')
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
        return false;
    }

    const QString key = serverKey();
    QLocalServer::removeServer(key);

    m_server = new QLocalServer(this);
    connect(m_server, &QLocalServer::newConnection, this, &SingleInstanceGuard::handleNewConnection);

    if (!m_server->listen(key)) {
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
