#include "MirrorEngine.hpp"
#include <QDir>
#include <QStandardPaths>
#include <QDataStream>
#include <QDebug>

MirrorEngine::MirrorEngine(QObject *parent)
    : QObject(parent)
    , m_serverProcess(new QProcess(this))
    , m_videoSocket(new QTcpSocket(this))
    , m_controlSocket(new QTcpSocket(this))
{
    connect(m_serverProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MirrorEngine::onProcessFinished);
    connect(m_videoSocket, &QTcpSocket::readyRead, this, &MirrorEngine::onSocketReadyRead);
}

MirrorEngine::~MirrorEngine() {
    stopMirroring();
}

bool MirrorEngine::isRunning() const {
    return m_isRunning;
}

int MirrorEngine::targetFps() const {
    return m_targetFps;
}

void MirrorEngine::setTargetFps(int fps) {
    if (m_targetFps != fps) {
        m_targetFps = fps;
        emit targetFpsChanged(fps);
    }
}

QString MirrorEngine::videoCodec() const {
    return m_videoCodec;
}

void MirrorEngine::setVideoCodec(const QString &codec) {
    if (m_videoCodec != codec) {
        m_videoCodec = codec;
        emit videoCodecChanged(codec);
    }
}

int MirrorEngine::videoBitrate() const {
    return m_videoBitrate;
}

void MirrorEngine::setVideoBitrate(int bitrate) {
    if (m_videoBitrate != bitrate) {
        m_videoBitrate = bitrate;
        emit videoBitrateChanged(bitrate);
    }
}

QString MirrorEngine::findAdb() const {
#if defined(Q_OS_WIN)
    if (QFile::exists(QStringLiteral("C:/Android/Sdk/platform-tools/adb.exe"))) {
        return QStringLiteral("C:/Android/Sdk/platform-tools/adb.exe");
    }
    return QStringLiteral("adb.exe");
#else
    return QStringLiteral("adb");
#endif
}

QString MirrorEngine::findNativeScrcpy() const {
#if defined(Q_OS_WIN)
    QString local = QStringLiteral("C:/Users/minua/AppData/Local/Microsoft/WinGet/Packages/Genymobile.scrcpy_Microsoft.Winget.Source_8wekyb3d8bbwe/scrcpy-win64-v5.0.1/scrcpy.exe");
    if (QFile::exists(local)) return local;
    return QStringLiteral("scrcpy.exe");
#else
    return QStringLiteral("scrcpy");
#endif
}

bool MirrorEngine::startMirroring(const QString &serial) {
    if (m_isRunning) stopMirroring();

    m_activeSerial = serial;
    QString scrcpyBin = findNativeScrcpy();

    QStringList args;
    if (!serial.isEmpty()) {
        args << QStringLiteral("-s") << serial;
    }

    // High refresh rate configuration (60, 90, 120 FPS)
    args << QStringLiteral("--max-fps=%1").arg(m_targetFps);

    // Low latency pipeline configuration
    args << QStringLiteral("--video-buffer=0");
    args << QStringLiteral("--video-codec=%1").arg(m_videoCodec);
    args << QStringLiteral("--video-bit-rate=%1M").arg(m_videoBitrate / 1000000);

    // Window & presentation properties
    args << QStringLiteral("--window-title=FluxMirror — %1 (%2 FPS)").arg(serial, QString::number(m_targetFps));
    args << QStringLiteral("--always-on-top");
    args << QStringLiteral("--stay-awake");
    args << QStringLiteral("--print-fps");

    m_serverProcess->setProgram(scrcpyBin);
    m_serverProcess->setArguments(args);
    m_serverProcess->setWorkingDirectory(QFileInfo(scrcpyBin).absolutePath());

    m_serverProcess->start();
    if (!m_serverProcess->waitForStarted(2000)) {
        emit engineError(QStringLiteral("Failed to launch mirroring process: ") + m_serverProcess->errorString());
        return false;
    }

    m_isRunning = true;
    emit runningChanged(true);
    return true;
}

void MirrorEngine::stopMirroring() {
    if (m_serverProcess && m_serverProcess->state() != QProcess::NotRunning) {
        m_serverProcess->kill();
        m_serverProcess->waitForFinished(1000);
    }
    if (m_videoSocket && m_videoSocket->isOpen()) {
        m_videoSocket->close();
    }
    if (m_controlSocket && m_controlSocket->isOpen()) {
        m_controlSocket->close();
    }
    m_isRunning = false;
    emit runningChanged(false);
}

void MirrorEngine::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    m_isRunning = false;
    emit runningChanged(false);
}

void MirrorEngine::onSocketReadyRead() {
    QByteArray data = m_videoSocket->readAll();
    emit frameReceived(data, 0);
}

void MirrorEngine::injectTouch(int action, int x, int y, int screenWidth, int screenHeight) {
    // Send ADB shell input event fallback if socket not open, or binary control packet
    if (!m_activeSerial.isEmpty()) {
        QString adb = findAdb();
        if (action == Scrcpy::AMOTION_EVENT_ACTION_DOWN || action == Scrcpy::AMOTION_EVENT_ACTION_UP) {
            QProcess::startDetached(adb, QStringList{
                QStringLiteral("-s"), m_activeSerial,
                QStringLiteral("shell"), QStringLiteral("input"),
                QStringLiteral("tap"), QString::number(x), QString::number(y)
            });
        }
    }
}

void MirrorEngine::injectKey(int action, int keycode) {
    Q_UNUSED(action);
    if (!m_activeSerial.isEmpty()) {
        QString adb = findAdb();
        QProcess::startDetached(adb, QStringList{
            QStringLiteral("-s"), m_activeSerial,
            QStringLiteral("shell"), QStringLiteral("input"),
            QStringLiteral("keyevent"), QString::number(keycode)
        });
    }
}

void MirrorEngine::injectScroll(int x, int y, int hScroll, int vScroll, int screenWidth, int screenHeight) {
    Q_UNUSED(screenWidth);
    Q_UNUSED(screenHeight);
    if (!m_activeSerial.isEmpty()) {
        QString adb = findAdb();
        int delta = vScroll > 0 ? 400 : -400;
        QProcess::startDetached(adb, QStringList{
            QStringLiteral("-s"), m_activeSerial,
            QStringLiteral("shell"), QStringLiteral("input"),
            QStringLiteral("swipe"),
            QString::number(x), QString::number(y),
            QString::number(x), QString::number(y + delta),
            QStringLiteral("150")
        });
    }
}

void MirrorEngine::rotateDevice() {
    if (!m_activeSerial.isEmpty()) {
        QString adb = findAdb();
        QProcess::startDetached(adb, QStringList{
            QStringLiteral("-s"), m_activeSerial,
            QStringLiteral("shell"), QStringLiteral("input"),
            QStringLiteral("keyevent"), QStringLiteral("130") // KEYCODE_REFRESH / Rotate
        });
    }
}

void MirrorEngine::copyDeviceClipboard() {
    if (!m_activeSerial.isEmpty()) {
        QString adb = findAdb();
        QProcess::startDetached(adb, QStringList{
            QStringLiteral("-s"), m_activeSerial,
            QStringLiteral("shell"), QStringLiteral("input"),
            QStringLiteral("keyevent"), QStringLiteral("278") // KEYCODE_COPY
        });
    }
}
