#include "PerformanceMonitor.hpp"

PerformanceMonitor::PerformanceMonitor(QObject *parent)
    : QObject(parent)
{
    m_timer.start();
    m_metricTimer = new QTimer(this);
    connect(m_metricTimer, &QTimer::timeout, this, &PerformanceMonitor::calculateMetrics);
    m_metricTimer->start(500); // Recalculate twice a second
}

qreal PerformanceMonitor::renderedFps() const {
    return m_renderedFps;
}

qreal PerformanceMonitor::decodedFps() const {
    return m_decodedFps;
}

quint64 PerformanceMonitor::droppedFrames() const {
    return m_droppedFrames;
}

qreal PerformanceMonitor::renderLatencyMs() const {
    return m_renderLatencyMs;
}

void PerformanceMonitor::recordDecodedFrame() {
    m_decodeTimestamps.enqueue(m_timer.elapsed());
}

void PerformanceMonitor::recordRenderedFrame(qreal latencyMs) {
    m_renderTimestamps.enqueue(m_timer.elapsed());
    if (latencyMs > 0.0) {
        m_renderLatencyMs = latencyMs;
        emit latencyUpdated();
    }
}

void PerformanceMonitor::recordFrameDrop() {
    m_droppedFrames++;
    emit frameDropped();
}

void PerformanceMonitor::calculateMetrics() {
    qint64 now = m_timer.elapsed();
    qint64 windowStart = now - 1000; // 1 second sliding window

    while (!m_renderTimestamps.isEmpty() && m_renderTimestamps.head() < windowStart) {
        m_renderTimestamps.dequeue();
    }
    while (!m_decodeTimestamps.isEmpty() && m_decodeTimestamps.head() < windowStart) {
        m_decodeTimestamps.dequeue();
    }

    m_renderedFps = static_cast<qreal>(m_renderTimestamps.size());
    m_decodedFps = static_cast<qreal>(m_decodeTimestamps.size());
    emit fpsUpdated();
}

void PerformanceMonitor::reset() {
    m_renderTimestamps.clear();
    m_decodeTimestamps.clear();
    m_renderedFps = 0.0;
    m_decodedFps = 0.0;
    m_droppedFrames = 0;
    m_renderLatencyMs = 0.0;
    emit fpsUpdated();
    emit frameDropped();
    emit latencyUpdated();
}
