#include "ClipboardManager.hpp"
#include <QGuiApplication>
#include <QStandardPaths>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

ClipboardManager::ClipboardManager(QObject *parent)
    : QObject(parent)
    , m_clipboard(QGuiApplication::clipboard())
{
    if (m_clipboard) {
        connect(m_clipboard, &QClipboard::dataChanged, this, &ClipboardManager::onSystemClipboardChanged);
    }
}

bool ClipboardManager::copyScreenshotToClipboard(const QImage &frame) {
    if (frame.isNull()) {
        emit clipboardError(QStringLiteral("Cannot copy empty frame to clipboard"));
        return false;
    }

    if (m_clipboard) {
        m_clipboard->setImage(frame, QClipboard::Clipboard);
        emit screenshotCopied();
        return true;
    }

    emit clipboardError(QStringLiteral("System clipboard unavailable"));
    return false;
}

bool ClipboardManager::saveScreenshotToFile(const QImage &frame, const QString &filePath) {
    if (frame.isNull()) {
        emit clipboardError(QStringLiteral("Cannot save empty frame"));
        return false;
    }

    QString targetPath = filePath;
    if (targetPath.isEmpty()) {
        targetPath = getDefaultScreenshotPath();
    }

    QDir().mkpath(QFileInfo(targetPath).absolutePath());
    if (frame.save(targetPath, "PNG")) {
        emit screenshotSaved(targetPath);
        return true;
    }

    emit clipboardError(QStringLiteral("Failed to write screenshot file to disk"));
    return false;
}

QString ClipboardManager::getDefaultScreenshotPath() const {
    QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (pictures.isEmpty()) {
        pictures = QDir::homePath();
    }
    QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"));
    return QDir(pictures).filePath(QStringLiteral("FluxMirror_%1.png").arg(timestamp));
}

void ClipboardManager::syncTextToDevice(const QString &text) {
    m_lastCopiedText = text;
    emit deviceClipboardChanged(text);
}

void ClipboardManager::onSystemClipboardChanged() {
    if (!m_clipboard) return;
    QString current = m_clipboard->text(QClipboard::Clipboard);
    if (!current.isEmpty() && current != m_lastCopiedText) {
        m_lastCopiedText = current;
        emit deviceClipboardChanged(current);
    }
}
