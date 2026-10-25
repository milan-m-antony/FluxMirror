#include "VideoDecoder.hpp"
#include <QDebug>

VideoDecoder::VideoDecoder(QObject *parent)
    : QObject(parent)
{
}

VideoDecoder::~VideoDecoder() {
    reset();
}

int VideoDecoder::videoWidth() const {
    return m_videoWidth;
}

int VideoDecoder::videoHeight() const {
    return m_videoHeight;
}

bool VideoDecoder::isHardwareAccelerated() const {
    return m_isHardwareAccelerated;
}

bool VideoDecoder::initDecoder(const QString &codecName) {
    Q_UNUSED(codecName);
    m_isHardwareAccelerated = true;
    return true;
}

void VideoDecoder::decodePacket(const QByteArray &packet, quint64 pts) {
    if (packet.isEmpty()) return;

    // Check for PNG header fallback (screencap buffer) or raw NALU
    if (packet.size() > 8 && static_cast<uint8_t>(packet.at(0)) == 0x89 && static_cast<uint8_t>(packet.at(1)) == 0x50) {
        QImage img;
        if (img.loadFromData(packet, "PNG")) {
            if (img.width() != m_videoWidth || img.height() != m_videoHeight) {
                m_videoWidth = img.width();
                m_videoHeight = img.height();
                emit videoSizeChanged(m_videoWidth, m_videoHeight);
            }
            emit frameDecoded(img, pts);
        }
    }
}

void VideoDecoder::reset() {
}
