#pragma once

#include <QObject>
#include <QQmlApplicationEngine>
#include <memory>

#include "../device/DeviceManager.hpp"
#include "../connection/ConnectionManager.hpp"
#include "../mirroring/MirrorEngine.hpp"
#include "../video/VideoDecoder.hpp"
#include "../rendering/VideoRenderer.hpp"
#include "../input/InputController.hpp"
#include "../clipboard/ClipboardManager.hpp"
#include "../window/WindowManager.hpp"
#include "../diagnostics/PerformanceMonitor.hpp"

class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(DeviceManager* deviceManager READ deviceManager CONSTANT)
    Q_PROPERTY(ConnectionManager* connectionManager READ connectionManager CONSTANT)
    Q_PROPERTY(MirrorEngine* mirrorEngine READ mirrorEngine CONSTANT)
    Q_PROPERTY(VideoDecoder* videoDecoder READ videoDecoder CONSTANT)
    Q_PROPERTY(InputController* inputController READ inputController CONSTANT)
    Q_PROPERTY(ClipboardManager* clipboardManager READ clipboardManager CONSTANT)
    Q_PROPERTY(WindowManager* windowManager READ windowManager CONSTANT)
    Q_PROPERTY(PerformanceMonitor* performanceMonitor READ performanceMonitor CONSTANT)

public:
    explicit AppController(QQmlApplicationEngine *engine, QObject *parent = nullptr);
    ~AppController() override = default;

    DeviceManager* deviceManager() const { return m_deviceManager; }
    ConnectionManager* connectionManager() const { return m_connectionManager; }
    MirrorEngine* mirrorEngine() const { return m_mirrorEngine; }
    VideoDecoder* videoDecoder() const { return m_videoDecoder; }
    InputController* inputController() const { return m_inputController; }
    ClipboardManager* clipboardManager() const { return m_clipboardManager; }
    WindowManager* windowManager() const { return m_windowManager; }
    PerformanceMonitor* performanceMonitor() const { return m_performanceMonitor; }

    Q_INVOKABLE void startSession(const QString &serial);
    Q_INVOKABLE void stopSession();
    Q_INVOKABLE void captureAndCopyScreenshot();

signals:
    void sessionStarted(const QString &serial);
    void sessionStopped();

private:
    QQmlApplicationEngine *m_qmlEngine = nullptr;

    DeviceManager *m_deviceManager = nullptr;
    ConnectionManager *m_connectionManager = nullptr;
    MirrorEngine *m_mirrorEngine = nullptr;
    VideoDecoder *m_videoDecoder = nullptr;
    InputController *m_inputController = nullptr;
    ClipboardManager *m_clipboardManager = nullptr;
    WindowManager *m_windowManager = nullptr;
    PerformanceMonitor *m_performanceMonitor = nullptr;
};
