#pragma once

#include <QObject>
#include <QString>
#include <QProcess>
#include <QTcpSocket>
#include <memory>
#include "ScrcpyProtocol.hpp"

class MirrorEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(int targetFps READ targetFps WRITE setTargetFps NOTIFY targetFpsChanged)
    Q_PROPERTY(QString videoCodec READ videoCodec WRITE setVideoCodec NOTIFY videoCodecChanged)
    Q_PROPERTY(int videoBitrate READ videoBitrate WRITE setVideoBitrate NOTIFY videoBitrateChanged)

public:
    explicit MirrorEngine(QObject *parent = nullptr);
    ~MirrorEngine() override;

    bool isRunning() const;
    int targetFps() const;
    void setTargetFps(int fps);

    QString videoCodec() const;
    void setVideoCodec(const QString &codec);

    int videoBitrate() const;
    void setVideoBitrate(int bitrate);

    Q_INVOKABLE bool startMirroring(const QString &serial);
    Q_INVOKABLE void stopMirroring();

    // Input injection
    Q_INVOKABLE void injectTouch(int action, int x, int y, int screenWidth, int screenHeight);
    Q_INVOKABLE void injectKey(int action, int keycode);
    Q_INVOKABLE void injectScroll(int x, int y, int hScroll, int vScroll, int screenWidth, int screenHeight);
    Q_INVOKABLE void rotateDevice();
    Q_INVOKABLE void copyDeviceClipboard();

signals:
    void runningChanged(bool running);
    void targetFpsChanged(int fps);
    void videoCodecChanged(const QString &codec);
    void videoBitrateChanged(int bitrate);
    void frameReceived(const QByteArray &frameData, quint64 pts);
    void engineError(const QString &error);

private slots:
    void onSocketReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QString findServerJar() const;
    QString findAdb() const;
    QString findNativeScrcpy() const;

    bool m_isRunning = false;
    int m_targetFps = 120;
    QString m_videoCodec = QStringLiteral("h264");
    int m_videoBitrate = 16000000; // 16 Mbps
    QString m_activeSerial;

    QProcess *m_serverProcess = nullptr;
    QTcpSocket *m_videoSocket = nullptr;
    QTcpSocket *m_controlSocket = nullptr;
};
