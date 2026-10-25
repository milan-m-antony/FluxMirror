#include "VideoRenderer.hpp"

VideoRenderer::VideoRenderer(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setAntialiasing(true);
}

qreal VideoRenderer::aspectRatio() const {
    return m_aspectRatio;
}

int VideoRenderer::frameWidth() const {
    return m_frameWidth;
}

int VideoRenderer::frameHeight() const {
    return m_frameHeight;
}

void VideoRenderer::updateFrame(const QImage &frame, quint64 pts) {
    Q_UNUSED(pts);
    m_currentFrame = frame;

    if (frame.width() != m_frameWidth || frame.height() != m_frameHeight) {
        m_frameWidth = frame.width();
        m_frameHeight = frame.height();
        m_aspectRatio = static_cast<qreal>(m_frameWidth) / static_cast<qreal>(m_frameHeight);
        emit frameSizeChanged(m_frameWidth, m_frameHeight);
        emit aspectRatioChanged(m_aspectRatio);
    }

    update();
}

void VideoRenderer::paint(QPainter *painter) {
    if (m_currentFrame.isNull()) {
        painter->fillRect(boundingRect(), Qt::black);
        return;
    }

    // Exact aspect-ratio preservation with letterbox / pillarbox centering
    const QRectF itemRect = boundingRect();
    const qreal targetRatio = m_aspectRatio;
    const qreal containerRatio = itemRect.width() / itemRect.height();

    QRectF targetRect;
    if (containerRatio > targetRatio) {
        // Container is wider than the phone -> Pillarbox (black bars on left & right)
        qreal renderWidth = itemRect.height() * targetRatio;
        qreal xOffset = (itemRect.width() - renderWidth) / 2.0;
        targetRect = QRectF(xOffset, 0, renderWidth, itemRect.height());
    } else {
        // Container is taller than the phone -> Letterbox (black bars on top & bottom)
        qreal renderHeight = itemRect.width() / targetRatio;
        qreal yOffset = (itemRect.height() - renderHeight) / 2.0;
        targetRect = QRectF(0, yOffset, itemRect.width(), renderHeight);
    }

    painter->fillRect(itemRect, Qt::black);
    painter->drawImage(targetRect, m_currentFrame);
}
