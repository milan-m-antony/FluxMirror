#include "WindowManager.hpp"
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>

WindowManager::WindowManager(QObject *parent)
    : QObject(parent)
{
}

bool WindowManager::alwaysOnTop() const {
    return m_alwaysOnTop;
}

void WindowManager::setAlwaysOnTop(bool top) {
    if (m_alwaysOnTop != top) {
        m_alwaysOnTop = top;
        if (m_window) {
            Qt::WindowFlags flags = m_window->flags();
            if (top) {
                flags |= Qt::WindowStaysOnTopHint;
            } else {
                flags &= ~Qt::WindowStaysOnTopHint;
            }
            m_window->setFlags(flags);
            m_window->show();
        }
        emit alwaysOnTopChanged(top);
    }
}

bool WindowManager::isFullscreen() const {
    return m_isFullscreen;
}

void WindowManager::setFullscreen(bool fs) {
    if (m_isFullscreen != fs) {
        m_isFullscreen = fs;
        if (m_window) {
            if (fs) {
                m_window->showFullScreen();
            } else {
                m_window->showNormal();
            }
        }
        emit fullscreenChanged(fs);
    }
}

void WindowManager::attachWindow(QQuickWindow *window) {
    m_window = window;
    if (m_window && m_alwaysOnTop) {
        m_window->setFlags(m_window->flags() | Qt::WindowStaysOnTopHint);
    }
}

void WindowManager::toggleAlwaysOnTop() {
    setAlwaysOnTop(!m_alwaysOnTop);
}

void WindowManager::toggleFullscreen() {
    setFullscreen(!m_isFullscreen);
}

void WindowManager::resizeToDeviceRatio(int deviceWidth, int deviceHeight) {
    if (!m_window || deviceWidth <= 0 || deviceHeight <= 0) return;

    qreal ratio = static_cast<qreal>(deviceWidth) / static_cast<qreal>(deviceHeight);
    int currentH = m_window->height();
    int idealW = qRound(currentH * ratio);

    m_window->resize(idealW, currentH);
}
