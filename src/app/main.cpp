#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QCommandLineParser>
#include <QIcon>
#include <QDebug>

#include "AppController.hpp"
#include "../rendering/VideoRenderer.hpp"

int main(int argc, char *argv[]) {
    // Enable High-DPI scaling
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Flux"));
    app.setOrganizationDomain(QStringLiteral("flux.org"));
    app.setApplicationName(QStringLiteral("FluxMirror"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    // Register custom QML types
    qmlRegisterType<VideoRenderer>("MirrorApp", 1, 0, "VideoRenderer");

    // Command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("High-Performance Native Cross-Platform Android Screen Mirror & Controller"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fpsOption(QStringList() << QStringLiteral("fps"),
        QStringLiteral("Target refresh rate (60, 90, 120, max)"), QStringLiteral("fps"), QStringLiteral("120"));
    QCommandLineOption wifiOption(QStringList() << QStringLiteral("wifi") << QStringLiteral("tcpip"),
        QStringLiteral("Enable wireless TCP/IP connection"));
    QCommandLineOption serialOption(QStringList() << QStringLiteral("s") << QStringLiteral("serial"),
        QStringLiteral("Specific device serial number"), QStringLiteral("serial"));

    parser.addOption(fpsOption);
    parser.addOption(wifiOption);
    parser.addOption(serialOption);
    parser.process(app);

    QQmlApplicationEngine engine;
    AppController controller(&engine);

    // Initial startup arguments
    if (parser.isSet(fpsOption)) {
        int fps = parser.value(fpsOption).toInt();
        if (fps > 0) controller.mirrorEngine()->setTargetFps(fps);
    }
    if (parser.isSet(wifiOption)) {
        controller.connectionManager()->setWifiMode(true);
    }

    // Load Main QML interface
    const QUrl url(QStringLiteral("qrc:/MirrorApp/src/ui/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    // Auto-start session if device is present
    QString targetSerial = parser.value(serialOption);
    controller.startSession(targetSerial);

    return app.exec();
}
