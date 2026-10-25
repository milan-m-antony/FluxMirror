#pragma once

#include <QObject>
#include <QImage>
#include <QString>
#include <QClipboard>

class ClipboardManager : public QObject {
    Q_OBJECT

public:
    explicit ClipboardManager(QObject *parent = nullptr);
    ~ClipboardManager() override = default;

    Q_INVOKABLE bool copyScreenshotToClipboard(const QImage &frame);
    Q_INVOKABLE bool saveScreenshotToFile(const QImage &frame, const QString &filePath);
    Q_INVOKABLE QString getDefaultScreenshotPath() const;
    Q_INVOKABLE void syncTextToDevice(const QString &text);

signals:
    void screenshotCopied();
    void screenshotSaved(const QString &path);
    void clipboardError(const QString &error);
    void deviceClipboardChanged(const QString &text);

private slots:
    void onSystemClipboardChanged();

private:
    QClipboard *m_clipboard = nullptr;
    QString m_lastCopiedText;
};
