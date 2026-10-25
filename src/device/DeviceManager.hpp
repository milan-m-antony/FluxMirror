#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QTimer>
#include <QProcess>

struct DisplayMode {
    int width = 1080;
    int height = 2340;
    int fps = 60;
};

struct DeviceInfo {
    QString serial;
    QString model;
    QString manufacturer;
    QString androidVersion;
    int width = 1080;
    int height = 2340;
    int maxRefreshRate = 60;
    QList<DisplayMode> supportedModes;
    QString ipAddress;
    bool isConnected = false;
};

class DeviceManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<QString> deviceSerials READ deviceSerials NOTIFY devicesChanged)
    Q_PROPERTY(QString activeSerial READ activeSerial WRITE setActiveSerial NOTIFY activeDeviceChanged)
    Q_PROPERTY(QString activeModel READ activeModel NOTIFY activeDeviceChanged)
    Q_PROPERTY(int activeMaxFps READ activeMaxFps NOTIFY activeDeviceChanged)
    Q_PROPERTY(int activeWidth READ activeWidth NOTIFY activeDeviceChanged)
    Q_PROPERTY(int activeHeight READ activeHeight NOTIFY activeDeviceChanged)

public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager() override = default;

    QList<QString> deviceSerials() const;
    QString activeSerial() const;
    void setActiveSerial(const QString &serial);

    QString activeModel() const;
    int activeMaxFps() const;
    int activeWidth() const;
    int activeHeight() const;

    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE DeviceInfo getDeviceInfo(const QString &serial) const;

signals:
    void devicesChanged();
    void activeDeviceChanged();
    void deviceConnected(const QString &serial);
    void deviceDisconnected(const QString &serial);

private slots:
    void onPollTimer();

private:
    void parseAdbDevicesOutput(const QString &output);
    DeviceInfo inspectDevice(const QString &serial);
    QString findAdbExecutable() const;

    QTimer *m_pollTimer = nullptr;
    QString m_adbPath;
    QList<QString> m_serials;
    QHash<QString, DeviceInfo> m_deviceCache;
    QString m_activeSerial;
};
