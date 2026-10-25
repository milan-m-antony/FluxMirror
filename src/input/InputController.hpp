#pragma once

#include <QObject>
#include <QPointF>
#include <QRectF>

class InputController : public QObject {
    Q_OBJECT

public:
    explicit InputController(QObject *parent = nullptr);
    ~InputController() override = default;

    // Normalizes viewport mouse coordinates to phone device coordinates
    Q_INVOKABLE QPointF mapToDevice(const QPointF &mousePos, const QRectF &viewportRect, int deviceWidth, int deviceHeight) const;

    Q_INVOKABLE void handlePointerDown(qreal x, qreal y, qreal viewW, qreal viewH, int devW, int devH);
    Q_INVOKABLE void handlePointerUp(qreal x, qreal y, qreal viewW, qreal viewH, int devW, int devH);
    Q_INVOKABLE void handlePointerMove(qreal x, qreal y, qreal viewW, qreal viewH, int devW, int devH);
    Q_INVOKABLE void handleWheel(qreal x, qreal y, int deltaY, qreal viewW, qreal viewH, int devW, int devH);

    // Android Navigation shortcuts
    Q_INVOKABLE void navigateBack();
    Q_INVOKABLE void navigateHome();
    Q_INVOKABLE void navigateRecents();
    Q_INVOKABLE void togglePower();

signals:
    void touchInjected(int action, int x, int y, int devW, int devH);
    void keyInjected(int action, int keycode);
    void scrollInjected(int x, int y, int hScroll, int vScroll, int devW, int devH);

private:
    QPointF m_pointerStart;
    qint64 m_pointerStartTime = 0;
};
