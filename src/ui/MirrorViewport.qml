import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MirrorApp 1.0

Rectangle {
    id: viewportRoot
    color: "#000000"

    // Video Surface
    VideoRenderer {
        id: renderer
        anchors.top: parent.top
        anchors.bottom: navBar.top
        anchors.left: parent.left
        anchors.right: parent.right

        // Interactive touch/mouse handling
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton

            onPressed: (mouse) => {
                if (mouse.button === Qt.RightButton) {
                    inputCtrl.navigateBack()
                } else if (mouse.button === Qt.MiddleButton) {
                    inputCtrl.navigateHome()
                } else {
                    inputCtrl.handlePointerDown(mouse.x, mouse.y, renderer.width, renderer.height, deviceMgr.activeWidth, deviceMgr.activeHeight)
                }
            }

            onReleased: (mouse) => {
                if (mouse.button === Qt.LeftButton) {
                    inputCtrl.handlePointerUp(mouse.x, mouse.y, renderer.width, renderer.height, deviceMgr.activeWidth, deviceMgr.activeHeight)
                }
            }

            onPositionChanged: (mouse) => {
                if (mouse.buttons & Qt.LeftButton) {
                    inputCtrl.handlePointerMove(mouse.x, mouse.y, renderer.width, renderer.height, deviceMgr.activeWidth, deviceMgr.activeHeight)
                }
            }

            onWheel: (wheel) => {
                inputCtrl.handleWheel(wheel.x, wheel.y, wheel.angleDelta.y, renderer.width, renderer.height, deviceMgr.activeWidth, deviceMgr.activeHeight)
            }
        }
    }

    // Android Bottom Navigation Bar
    Rectangle {
        id: navBar
        anchors.bottom: parent.bottom
        width: parent.width
        height: 40
        color: "#000000"

        Rectangle {
            anchors.top: parent.top
            width: parent.width
            height: 1
            color: "#1e293b"
        }

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // Back Button
            Button {
                Layout.fillWidth: true
                Layout.fillHeight: true
                background: Rectangle { color: "transparent" }
                contentItem: Text {
                    text: "◀"
                    color: "#94a3b8"
                    font.pixelSize: 18
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: inputCtrl.navigateBack()
            }

            // Home Button
            Button {
                Layout.fillWidth: true
                Layout.fillHeight: true
                background: Rectangle { color: "transparent" }
                contentItem: Text {
                    text: "●"
                    color: "#94a3b8"
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: inputCtrl.navigateHome()
            }

            // Recents Button
            Button {
                Layout.fillWidth: true
                Layout.fillHeight: true
                background: Rectangle { color: "transparent" }
                contentItem: Text {
                    text: "■"
                    color: "#94a3b8"
                    font.pixelSize: 15
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: inputCtrl.navigateRecents()
            }
        }
    }
}
