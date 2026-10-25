#pragma once

#include <QQuickPaintedItem>
#include <QImage>
#include <QPainter>

class VideoRenderer : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(qreal aspectRatio READ aspectRatio NOTIFY aspectRatioChanged)
    Q_PROPERTY(int frameWidth READ frameWidth NOTIFY frameSizeChanged)
    Q_PROPERTY(int frameHeight READ frameHeight NOTIFY frameSizeChanged)

public:
    explicit VideoRenderer(QQuickItem *parent = nullptr);
    ~VideoRenderer() override = default;

    qreal aspectRatio() const;
    int frameWidth() const;
    int frameHeight() const;

    void paint(QPainter *painter) override;

public slots:
    void updateFrame(const QImage &frame, quint64 pts);

signals:
    void aspectRatioChanged(qreal ratio);
    void frameSizeChanged(int width, int height);

private:
    QImage m_currentFrame;
    int m_frameWidth = 1080;
    int m_frameHeight = 2340;
    qreal m_aspectRatio = 1080.0 / 2340.0;
};
