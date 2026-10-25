#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>
#include <QQueue>

class PerformanceMonitor : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal renderedFps READ renderedFps NOTIFY fpsUpdated)
    Q_PROPERTY(qreal decodedFps READ decodedFps NOTIFY fpsUpdated)
    Q_PROPERTY(quint64 droppedFrames READ droppedFrames NOTIFY frameDropped)
    Q_PROPERTY(qreal renderLatencyMs READ renderLatencyMs NOTIFY latencyUpdated)

public:
    explicit PerformanceMonitor(QObject *parent = nullptr);
    ~PerformanceMonitor() override = default;

    qreal renderedFps() const;
    qreal decodedFps() const;
    quint64 droppedFrames() const;
    qreal renderLatencyMs() const;

    Q_INVOKABLE void recordDecodedFrame();
    Q_INVOKABLE void recordRenderedFrame(qreal latencyMs = 0.0);
    Q_INVOKABLE void recordFrameDrop();
    Q_INVOKABLE void reset();

signals:
    void fpsUpdated();
    void frameDropped();
    void latencyUpdated();

private slots:
    void calculateMetrics();

private:
    QTimer *m_metricTimer = nullptr;
    QQueue<qint64> m_renderTimestamps;
    QQueue<qint64> m_decodeTimestamps;
    QElapsedTimer m_timer;

    qreal m_renderedFps = 0.0;
    qreal m_decodedFps = 0.0;
    quint64 m_droppedFrames = 0;
    qreal m_renderLatencyMs = 0.0;
};
