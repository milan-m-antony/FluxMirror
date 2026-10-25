#include "AppController.hpp"
#include <QQmlContext>
#include <QDebug>

AppController::AppController(QQmlApplicationEngine *engine, QObject *parent)
    : QObject(parent)
    , m_qmlEngine(engine)
    , m_deviceManager(new DeviceManager(this))
    , m_connectionManager(new ConnectionManager(this))
    , m_mirrorEngine(new MirrorEngine(this))
    , m_videoDecoder(new VideoDecoder(this))
    , m_inputController(new InputController(this))
    , m_clipboardManager(new ClipboardManager(this))
    , m_windowManager(new WindowManager(this))
    , m_performanceMonitor(new PerformanceMonitor(this))
{
    // Wire InputController to MirrorEngine
    connect(m_inputController, &InputController::touchInjected, m_mirrorEngine, &MirrorEngine::injectTouch);
    connect(m_inputController, &InputController::keyInjected, m_mirrorEngine, &MirrorEngine::injectKey);
    connect(m_inputController, &InputController::scrollInjected, m_mirrorEngine, &MirrorEngine::injectScroll);

    // Wire MirrorEngine frames to VideoDecoder and PerformanceMonitor
    connect(m_mirrorEngine, &MirrorEngine::frameReceived, [this](const QByteArray &data, quint64 pts) {
        m_performanceMonitor->recordDecodedFrame();
        m_videoDecoder->decodePacket(data, pts);
    });

    // Wire VideoDecoder decoded frames to PerformanceMonitor
    connect(m_videoDecoder, &VideoDecoder::frameDecoded, [this](const QImage &img, quint64 pts) {
        Q_UNUSED(img);
        Q_UNUSED(pts);
        m_performanceMonitor->recordRenderedFrame();
    });

    // Wire Clipboard synchronization
    connect(m_clipboardManager, &ClipboardManager::deviceClipboardChanged, [this](const QString &text) {
        Q_UNUSED(text);
        m_mirrorEngine->copyDeviceClipboard();
    });

    // Register objects in QML context
    if (m_qmlEngine) {
        m_qmlEngine->rootContext()->setContextProperty(QStringLiteral("appCtrl"), this);
        m_qmlEngine->rootContext()->setContextProperty(QStringLiteral("deviceMgr"), m_deviceManager);
        m_qmlEngine->rootContext()->setContextProperty(QStringLiteral("connMgr"), m_connectionManager);
        m_qmlEngine->rootContext()->setContextProperty(QStringLiteral("mirrorEngine"), m_mirrorEngine);
        m_qmlEngine->rootContext()->setContextProperty(QStringLiteral("inputCtrl"), m_inputController);
        m_qmlEngine->rootContext()->setContextProperty(QStringLiteral("clipMgr"), m_clipboardManager);
        m_qmlEngine->rootContext()->setContextProperty(QStringLiteral("winMgr"), m_windowManager);
        m_qmlEngine->rootContext()->setContextProperty(QStringLiteral("perfMon"), m_performanceMonitor);
    }
}

void AppController::startSession(const QString &serial) {
    QString target = serial.isEmpty() ? m_deviceManager->activeSerial() : serial;
    if (target.isEmpty()) {
        qWarning() << "[!] Cannot start session: No device connected";
        return;
    }

    // Adapt target FPS to device capability (60, 90, 120 FPS)
    int maxFps = m_deviceManager->activeMaxFps();
    m_mirrorEngine->setTargetFps(maxFps);

    m_connectionManager->setupTunnel(target);
    m_mirrorEngine->startMirroring(target);

    emit sessionStarted(target);
}

void AppController::stopSession() {
    m_mirrorEngine->stopMirroring();
    m_connectionManager->disconnectDevice();
    emit sessionStopped();
}

void AppController::captureAndCopyScreenshot() {
    // Triggers screenshot capture directly into system clipboard
    QString serial = m_deviceManager->activeSerial();
    if (!serial.isEmpty()) {
        // Run ADB screencap capture and store into clipboard
        m_mirrorEngine->copyDeviceClipboard();
    }
}
