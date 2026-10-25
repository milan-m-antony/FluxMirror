#pragma once

#include <QObject>
#include <QWindow>
#include <QQuickWindow>

class WindowManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool alwaysOnTop READ alwaysOnTop WRITE setAlwaysOnTop NOTIFY alwaysOnTopChanged)
    Q_PROPERTY(bool isFullscreen READ isFullscreen WRITE setFullscreen NOTIFY fullscreenChanged)

public:
    explicit WindowManager(QObject *parent = nullptr);
    ~WindowManager() override = default;

    bool alwaysOnTop() const;
    void setAlwaysOnTop(bool top);

    bool isFullscreen() const;
    void setFullscreen(bool fs);

    Q_INVOKABLE void attachWindow(QQuickWindow *window);
    Q_INVOKABLE void toggleAlwaysOnTop();
    Q_INVOKABLE void toggleFullscreen();
    Q_INVOKABLE void resizeToDeviceRatio(int deviceWidth, int deviceHeight);

signals:
    void alwaysOnTopChanged(bool top);
    void fullscreenChanged(bool fs);

private:
    QQuickWindow *m_window = nullptr;
    bool m_alwaysOnTop = true;
    bool m_isFullscreen = false;
};
