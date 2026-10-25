import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MirrorApp 1.0

ApplicationWindow {
    id: rootWindow
    visible: true
    width: 390
    height: 840
    minimumWidth: 260
    minimumHeight: 480
    title: qsTr("FluxMirror — Native Device Mirror")
    color: "#000000"

    Component.onCompleted: {
        winMgr.attachWindow(rootWindow)
    }

    // Key events for global shortcuts
    Item {
        focus: true
        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_F11) {
                winMgr.toggleFullscreen()
                event.accepted = true
            } else if (event.modifiers & Qt.AltModifier && event.key === Qt.Key_R) {
                mirrorEngine.rotateDevice()
                event.accepted = true
            } else if (event.modifiers & Qt.AltModifier && event.key === Qt.Key_S) {
                inputCtrl.navigateRecents()
                event.accepted = true
            } else if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
                inputCtrl.navigateBack()
                event.accepted = true
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 1. Top Control Toolbar
        DeviceToolbar {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
        }

        // 2. Video Viewport & Controls
        MirrorViewport {
            id: viewport
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    // Performance Telemetry Overlay (Toggle with F3 or click)
    PerformanceOverlay {
        anchors.top: parent.top
        anchors.topMargin: 46
        anchors.right: parent.right
        anchors.rightMargin: 10
        visible: true
    }
}
