#include "InputController.hpp"
#include "../mirroring/ScrcpyProtocol.hpp"
#include <QDateTime>
#include <algorithm>
#include <cmath>

InputController::InputController(QObject *parent)
    : QObject(parent)
{
}

QPointF InputController::mapToDevice(const QPointF &mousePos, const QRectF &viewportRect, int deviceWidth, int deviceHeight) const {
    if (viewportRect.isEmpty() || deviceWidth <= 0 || deviceHeight <= 0) {
        return QPointF(0, 0);
    }

    const qreal targetRatio = static_cast<qreal>(deviceWidth) / static_cast<qreal>(deviceHeight);
    const qreal containerRatio = viewportRect.width() / viewportRect.height();

    QRectF renderRect;
    if (containerRatio > targetRatio) {
        // Pillarbox
        qreal renderWidth = viewportRect.height() * targetRatio;
        qreal xOffset = (viewportRect.width() - renderWidth) / 2.0;
        renderRect = QRectF(xOffset, 0, renderWidth, viewportRect.height());
    } else {
        // Letterbox
        qreal renderHeight = viewportRect.width() / targetRatio;
        qreal yOffset = (viewportRect.height() - renderHeight) / 2.0;
        renderRect = QRectF(0, yOffset, viewportRect.width(), renderHeight);
    }

    if (!renderRect.contains(mousePos)) {
        // Clamp to nearest border
        qreal clampedX = std::clamp(mousePos.x(), renderRect.left(), renderRect.right());
        qreal clampedY = std::clamp(mousePos.y(), renderRect.top(), renderRect.bottom());
        qreal devX = ((clampedX - renderRect.left()) / renderRect.width()) * deviceWidth;
        qreal devY = ((clampedY - renderRect.top()) / renderRect.height()) * deviceHeight;
        return QPointF(devX, devY);
    }

    qreal normalizedX = (mousePos.x() - renderRect.left()) / renderRect.width();
    qreal normalizedY = (mousePos.y() - renderRect.top()) / renderRect.height();

    qreal deviceX = std::clamp(normalizedX * deviceWidth, 0.0, static_cast<qreal>(deviceWidth));
    qreal deviceY = std::clamp(normalizedY * deviceHeight, 0.0, static_cast<qreal>(deviceHeight));

    return QPointF(deviceX, deviceY);
}

void InputController::handlePointerDown(qreal x, qreal y, qreal viewW, qreal viewH, int devW, int devH) {
    QPointF devCoords = mapToDevice(QPointF(x, y), QRectF(0, 0, viewW, viewH), devW, devH);
    m_pointerStart = devCoords;
    m_pointerStartTime = QDateTime::currentMSecsSinceEpoch();

    emit touchInjected(Scrcpy::AMOTION_EVENT_ACTION_DOWN, qRound(devCoords.x()), qRound(devCoords.y()), devW, devH);
}

void InputController::handlePointerUp(qreal x, qreal y, qreal viewW, qreal viewH, int devW, int devH) {
    QPointF endCoords = mapToDevice(QPointF(x, y), QRectF(0, 0, viewW, viewH), devW, devH);
    qint64 duration = QDateTime::currentMSecsSinceEpoch() - m_pointerStartTime;
    Q_UNUSED(duration);

    emit touchInjected(Scrcpy::AMOTION_EVENT_ACTION_UP, qRound(endCoords.x()), qRound(endCoords.y()), devW, devH);
}

void InputController::handlePointerMove(qreal x, qreal y, qreal viewW, qreal viewH, int devW, int devH) {
    QPointF devCoords = mapToDevice(QPointF(x, y), QRectF(0, 0, viewW, viewH), devW, devH);
    emit touchInjected(Scrcpy::AMOTION_EVENT_ACTION_MOVE, qRound(devCoords.x()), qRound(devCoords.y()), devW, devH);
}

void InputController::handleWheel(qreal x, qreal y, int deltaY, qreal viewW, qreal viewH, int devW, int devH) {
    QPointF devCoords = mapToDevice(QPointF(x, y), QRectF(0, 0, viewW, viewH), devW, devH);
    emit scrollInjected(qRound(devCoords.x()), qRound(devCoords.y()), 0, deltaY, devW, devH);
}

void InputController::navigateBack() {
    emit keyInjected(Scrcpy::AKEY_EVENT_ACTION_DOWN, Scrcpy::AKEYCODE_BACK);
    emit keyInjected(Scrcpy::AKEY_EVENT_ACTION_UP, Scrcpy::AKEYCODE_BACK);
}

void InputController::navigateHome() {
    emit keyInjected(Scrcpy::AKEY_EVENT_ACTION_DOWN, Scrcpy::AKEYCODE_HOME);
    emit keyInjected(Scrcpy::AKEY_EVENT_ACTION_UP, Scrcpy::AKEYCODE_HOME);
}

void InputController::navigateRecents() {
    emit keyInjected(Scrcpy::AKEY_EVENT_ACTION_DOWN, Scrcpy::AKEYCODE_APP_SWITCH);
    emit keyInjected(Scrcpy::AKEY_EVENT_ACTION_UP, Scrcpy::AKEYCODE_APP_SWITCH);
}

void InputController::togglePower() {
    emit keyInjected(Scrcpy::AKEY_EVENT_ACTION_DOWN, Scrcpy::AKEYCODE_POWER);
    emit keyInjected(Scrcpy::AKEY_EVENT_ACTION_UP, Scrcpy::AKEYCODE_POWER);
}
