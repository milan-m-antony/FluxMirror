import QtQuick
import QtQuick.Layouts

Rectangle {
    id: overlay
    width: layout.implicitWidth + 16
    height: layout.implicitHeight + 10
    radius: 6
    color: "#d90f172a"
    border.color: "#334155"
    border.width: 1

    ColumnLayout {
        id: layout
        anchors.centerIn: parent
        spacing: 2

        RowLayout {
            spacing: 6
            Text {
                text: "FPS:"
                color: "#64748b"
                font.pixelSize: 10
                font.bold: true
            }
            Text {
                text: Math.round(perfMon.renderedFps) + " / " + mirrorEngine.targetFps
                color: "#10b981"
                font.pixelSize: 11
                font.bold: true
            }
        }

        RowLayout {
            spacing: 6
            Text {
                text: "Drops:"
                color: "#64748b"
                font.pixelSize: 10
            }
            Text {
                text: perfMon.droppedFrames.toString()
                color: perfMon.droppedFrames > 0 ? "#ef4444" : "#94a3b8"
                font.pixelSize: 10
            }
        }

        RowLayout {
            spacing: 6
            Text {
                text: "Pipeline:"
                color: "#64748b"
                font.pixelSize: 10
            }
            Text {
                text: mirrorEngine.videoCodec.toUpperCase() + " (Direct GPU)"
                color: "#38bdf8"
                font.pixelSize: 10
            }
        }
    }
}
