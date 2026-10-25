#pragma once

#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTcpServer>

class ConnectionManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStatusChanged)
    Q_PROPERTY(bool isWifiMode READ isWifiMode WRITE setWifiMode NOTIFY wifiModeChanged)
    Q_PROPERTY(quint16 localPort READ localPort NOTIFY portChanged)

public:
    explicit ConnectionManager(QObject *parent = nullptr);
    ~ConnectionManager() override;

    bool isConnected() const;
    bool isWifiMode() const;
    void setWifiMode(bool wifi);
    quint16 localPort() const;

    Q_INVOKABLE bool setupTunnel(const QString &serial, quint16 port = 27183);
    Q_INVOKABLE bool enableWirelessAdb(const QString &serial, quint16 port = 5555);
    Q_INVOKABLE void disconnectDevice();

signals:
    void connectionStatusChanged(bool connected);
    void wifiModeChanged(bool wifi);
    void portChanged(quint16 port);
    void connectionError(const QString &errorMessage);

private:
    QString m_adbPath;
    QString m_currentSerial;
    quint16 m_localPort = 27183;
    bool m_isConnected = false;
    bool m_isWifiMode = false;
};
