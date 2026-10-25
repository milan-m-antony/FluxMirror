#pragma once

#include <QObject>
#include <QImage>
#include <QByteArray>
#include <memory>

class VideoDecoder : public QObject {
    Q_OBJECT
    Q_PROPERTY(int videoWidth READ videoWidth NOTIFY videoSizeChanged)
    Q_PROPERTY(int videoHeight READ videoHeight NOTIFY videoSizeChanged)
    Q_PROPERTY(bool isHardwareAccelerated READ isHardwareAccelerated CONSTANT)

public:
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder() override;

    int videoWidth() const;
    int videoHeight() const;
    bool isHardwareAccelerated() const;

    Q_INVOKABLE bool initDecoder(const QString &codecName = QStringLiteral("h264"));
    Q_INVOKABLE void decodePacket(const QByteArray &packet, quint64 pts);
    Q_INVOKABLE void reset();

signals:
    void videoSizeChanged(int width, int height);
    void frameDecoded(const QImage &frame, quint64 pts);
    void decoderError(const QString &error);

private:
    int m_videoWidth = 1080;
    int m_videoHeight = 2340;
    bool m_isHardwareAccelerated = true;
};
