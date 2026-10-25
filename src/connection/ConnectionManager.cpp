#include "ConnectionManager.hpp"
#include <QProcess>
#include <QDir>
#include <QDebug>

ConnectionManager::ConnectionManager(QObject *parent)
    : QObject(parent)
{
#if defined(Q_OS_WIN)
    m_adbPath = QStringLiteral("adb.exe");
    if (QFile::exists(QStringLiteral("C:/Android/Sdk/platform-tools/adb.exe"))) {
        m_adbPath = QStringLiteral("C:/Android/Sdk/platform-tools/adb.exe");
    }
#else
    m_adbPath = QStringLiteral("adb");
#endif
}

ConnectionManager::~ConnectionManager() {
    disconnectDevice();
}

bool ConnectionManager::isConnected() const {
    return m_isConnected;
}

bool ConnectionManager::isWifiMode() const {
    return m_isWifiMode;
}

void ConnectionManager::setWifiMode(bool wifi) {
    if (m_isWifiMode != wifi) {
        m_isWifiMode = wifi;
        emit wifiModeChanged(wifi);
    }
}

quint16 ConnectionManager::localPort() const {
    return m_localPort;
}

bool ConnectionManager::setupTunnel(const QString &serial, quint16 port) {
    if (serial.isEmpty()) {
        emit connectionError(QStringLiteral("No device serial provided"));
        return false;
    }

    m_currentSerial = serial;
    m_localPort = port;

    // Use adb forward to bridge desktop local TCP port to device scrcpy abstract socket
    QProcess process;
    process.start(m_adbPath, QStringList{
        QStringLiteral("-s"), serial,
        QStringLiteral("forward"),
        QStringLiteral("tcp:%1").arg(m_localPort),
        QStringLiteral("localabstract:scrcpy")
    });

    if (!process.waitForFinished(2000) || process.exitCode() != 0) {
        QString err = QString::fromUtf8(process.readAllStandardError());
        emit connectionError(QStringLiteral("Failed to setup ADB forward tunnel: ") + err);
        return false;
    }

    m_isConnected = true;
    emit connectionStatusChanged(true);
    emit portChanged(m_localPort);
    return true;
}

bool ConnectionManager::enableWirelessAdb(const QString &serial, quint16 port) {
    QProcess process;
    process.start(m_adbPath, QStringList{
        QStringLiteral("-s"), serial,
        QStringLiteral("tcpip"),
        QString::number(port)
    });

    if (!process.waitForFinished(3000) || process.exitCode() != 0) {
        emit connectionError(QStringLiteral("Failed to enable wireless TCP/IP mode on device"));
        return false;
    }

    setWifiMode(true);
    return true;
}

void ConnectionManager::disconnectDevice() {
    if (m_isConnected && !m_currentSerial.isEmpty()) {
        QProcess process;
        process.start(m_adbPath, QStringList{
            QStringLiteral("-s"), m_currentSerial,
            QStringLiteral("forward"),
            QStringLiteral("--remove"),
            QStringLiteral("tcp:%1").arg(m_localPort)
        });
        process.waitForFinished(1000);

        m_isConnected = false;
        m_currentSerial.clear();
        emit connectionStatusChanged(false);
    }
}
