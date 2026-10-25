#include "DeviceManager.hpp"
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
    , m_adbPath(findAdbExecutable())
{
    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &DeviceManager::onPollTimer);
    m_pollTimer->start(2500);

    refreshDevices();
}

QString DeviceManager::findAdbExecutable() const {
#if defined(Q_OS_WIN)
    QStringList candidates = {
        QStringLiteral("C:/Android/Sdk/platform-tools/adb.exe"),
        QDir::homePath() + QStringLiteral("/AppData/Local/Android/Sdk/platform-tools/adb.exe"),
        QStringLiteral("adb.exe")
    };
#elif defined(Q_OS_MAC)
    QStringList candidates = {
        QStringLiteral("/opt/homebrew/bin/adb"),
        QStringLiteral("/usr/local/bin/adb"),
        QDir::homePath() + QStringLiteral("/Library/Android/sdk/platform-tools/adb"),
        QStringLiteral("adb")
    };
#else
    QStringList candidates = {
        QStringLiteral("/usr/bin/adb"),
        QStringLiteral("/usr/local/bin/adb"),
        QStringLiteral("adb")
    };
#endif

    for (const auto &cand : candidates) {
        if (QFile::exists(cand)) return cand;
    }
    return QStringLiteral("adb");
}

QList<QString> DeviceManager::deviceSerials() const {
    return m_serials;
}

QString DeviceManager::activeSerial() const {
    return m_activeSerial;
}

void DeviceManager::setActiveSerial(const QString &serial) {
    if (m_activeSerial != serial) {
        m_activeSerial = serial;
        emit activeDeviceChanged();
    }
}

QString DeviceManager::activeModel() const {
    if (m_deviceCache.contains(m_activeSerial)) {
        return m_deviceCache[m_activeSerial].model;
    }
    return QStringLiteral("Android Device");
}

int DeviceManager::activeMaxFps() const {
    if (m_deviceCache.contains(m_activeSerial)) {
        return m_deviceCache[m_activeSerial].maxRefreshRate;
    }
    return 60;
}

int DeviceManager::activeWidth() const {
    if (m_deviceCache.contains(m_activeSerial)) {
        return m_deviceCache[m_activeSerial].width;
    }
    return 1080;
}

int DeviceManager::activeHeight() const {
    if (m_deviceCache.contains(m_activeSerial)) {
        return m_deviceCache[m_activeSerial].height;
    }
    return 2340;
}

void DeviceManager::onPollTimer() {
    refreshDevices();
}

void DeviceManager::refreshDevices() {
    QProcess process;
    process.start(m_adbPath, QStringList{QStringLiteral("devices"), QStringLiteral("-l")});
    if (!process.waitForFinished(1500)) return;

    QString output = QString::fromUtf8(process.readAllStandardOutput());
    parseAdbDevicesOutput(output);
}

void DeviceManager::parseAdbDevicesOutput(const QString &output) {
    QList<QString> detected;
    const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(QStringLiteral("List of"))) continue;

        QStringList parts = trimmed.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (parts.size() >= 2 && parts[1] == QStringLiteral("device")) {
            QString serial = parts[0];
            detected.append(serial);

            if (!m_deviceCache.contains(serial)) {
                m_deviceCache[serial] = inspectDevice(serial);
                emit deviceConnected(serial);
            }
        }
    }

    // Check for disconnected devices
    for (const QString &oldSerial : m_serials) {
        if (!detected.contains(oldSerial)) {
            m_deviceCache.remove(oldSerial);
            emit deviceDisconnected(oldSerial);
        }
    }

    if (m_serials != detected) {
        m_serials = detected;
        emit devicesChanged();

        if (m_activeSerial.isEmpty() || !m_serials.contains(m_activeSerial)) {
            if (!m_serials.isEmpty()) {
                setActiveSerial(m_serials.first());
            } else {
                setActiveSerial(QString());
            }
        }
    }
}

DeviceInfo DeviceManager::inspectDevice(const QString &serial) {
    DeviceInfo info;
    info.serial = serial;
    info.isConnected = true;

    // 1. Model & Manufacturer
    QProcess p;
    p.start(m_adbPath, QStringList{QStringLiteral("-s"), serial, QStringLiteral("shell"), QStringLiteral("getprop"), QStringLiteral("ro.product.model")});
    if (p.waitForFinished(1000)) info.model = QString::fromUtf8(p.readAllStandardOutput()).trimmed();

    p.start(m_adbPath, QStringList{QStringLiteral("-s"), serial, QStringLiteral("shell"), QStringLiteral("getprop"), QStringLiteral("ro.product.manufacturer")});
    if (p.waitForFinished(1000)) info.manufacturer = QString::fromUtf8(p.readAllStandardOutput()).trimmed();

    p.start(m_adbPath, QStringList{QStringLiteral("-s"), serial, QStringLiteral("shell"), QStringLiteral("getprop"), QStringLiteral("ro.build.version.release")});
    if (p.waitForFinished(1000)) info.androidVersion = QString::fromUtf8(p.readAllStandardOutput()).trimmed();

    // 2. Physical Screen Size
    p.start(m_adbPath, QStringList{QStringLiteral("-s"), serial, QStringLiteral("shell"), QStringLiteral("wm"), QStringLiteral("size")});
    if (p.waitForFinished(1000)) {
        QString sizeOut = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
        static const QRegularExpression sizeRegex(QStringLiteral("(\\d+)x(\\d+)"));
        auto match = sizeRegex.match(sizeOut);
        if (match.hasMatch()) {
            info.width = match.captured(1).toInt();
            info.height = match.captured(2).toInt();
        }
    }

    // 3. Supported Display Refresh Rates (60Hz, 90Hz, 120Hz)
    info.maxRefreshRate = 60;
    p.start(m_adbPath, QStringList{QStringLiteral("-s"), serial, QStringLiteral("shell"), QStringLiteral("dumpsys"), QStringLiteral("display")});
    if (p.waitForFinished(1500)) {
        QString dump = QString::fromUtf8(p.readAllStandardOutput());
        static const QRegularExpression fpsRegex(QStringLiteral("fps=([0-9.]+)"));
        auto it = fpsRegex.globalMatch(dump);
        while (it.hasNext()) {
            auto m = it.next();
            int fps = qRound(m.captured(1).toDouble());
            if (fps >= 30 && fps <= 240) {
                if (fps > info.maxRefreshRate) info.maxRefreshRate = fps;
                bool exists = false;
                for (const auto &mode : info.supportedModes) {
                    if (mode.fps == fps) { exists = true; break; }
                }
                if (!exists) {
                    info.supportedModes.append(DisplayMode{info.width, info.height, fps});
                }
            }
        }
    }

    return info;
}

DeviceInfo DeviceManager::getDeviceInfo(const QString &serial) const {
    return m_deviceCache.value(serial);
}
