import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: toolbar
    color: "#0f172a"

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: "#1e293b"
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 8

        // Connection Dot
        Rectangle {
            width: 8
            height: 8
            radius: 4
            color: deviceMgr.activeSerial !== "" ? "#10b981" : "#ef4444"
        }

        // Device Model Label
        Text {
            text: deviceMgr.activeModel
            color: "#94a3b8"
            font.pixelSize: 12
            font.weight: Font.DemiBold
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        // FPS Badge
        Rectangle {
            color: "#1e293b"
            radius: 4
            implicitWidth: fpsLabel.implicitWidth + 10
            implicitHeight: 20

            Text {
                id: fpsLabel
                anchors.centerIn: parent
                text: mirrorEngine.targetFps + " FPS"
                color: "#ff6b00"
                font.pixelSize: 10
                font.bold: true
            }
        }

        // Copy Screenshot Button (Prominent)
        Button {
            id: snapBtn
            implicitHeight: 26
            padding: 8

            background: Rectangle {
                radius: 13
                color: snapBtn.down ? "#e05500" : (snapBtn.hovered ? "#ff8533" : "#ff6b00")
            }

            contentItem: RowLayout {
                spacing: 4
                Text {
                    text: "📷 Copy Screenshot"
                    color: "#ffffff"
                    font.pixelSize: 11
                    font.bold: true
                }
            }

            onClicked: {
                appCtrl.captureAndCopyScreenshot()
            }
        }

        // Rotate Button
        Button {
            id: rotBtn
            implicitWidth: 26
            implicitHeight: 26
            background: Rectangle {
                radius: 13
                color: rotBtn.hovered ? "#334155" : "transparent"
            }
            contentItem: Text {
                text: "🔄"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 12
            }
            onClicked: {
                mirrorEngine.rotateDevice()
            }
        }

        // Always On Top Pin Toggle
        Button {
            id: pinBtn
            implicitWidth: 26
            implicitHeight: 26
            background: Rectangle {
                radius: 13
                color: winMgr.alwaysOnTop ? "#334155" : "transparent"
                border.color: winMgr.alwaysOnTop ? "#ff6b00" : "transparent"
                border.width: 1
            }
            contentItem: Text {
                text: "📌"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 12
            }
            onClicked: {
                winMgr.toggleAlwaysOnTop()
            }
        }
    }
}
