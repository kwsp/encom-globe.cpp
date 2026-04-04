import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: "QtQuick Playground"

    Label {
        anchors.centerIn: parent
        text: "Hello, QtQuick!"
        font.pixelSize: 32
    }
}
