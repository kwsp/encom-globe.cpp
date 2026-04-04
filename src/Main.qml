import QtQuick
import QtQuick.Controls
import EncomGlobe

ApplicationWindow {
    id: root
    visible: true
    width: 1200
    height: 800
    title: "Encom Globe"
    color: "#000000"
    
    Globe {
        id: globe
        anchors.fill: parent
        
        dayLength: 28000  // 28 seconds per rotation
        scale: 1.0
        viewAngle: 0.1
        baseColor: "#ffcc00"
        introDuration: 2000
        
        // Add some test satellites after intro
        Timer {
            interval: 2500  // After intro animation
            running: true
            repeat: false
            onTriggered: {
                globe.addSatellite(40.7128, -74.0060, 1.2)  // New York
                // globe.addSatellite(51.5074, -0.1278, 1.3)   // London
                // globe.addSatellite(35.6762, 139.6503, 1.15) // Tokyo
                globe.addSatellite(-33.8688, 151.2093, 1.25) // Sydney
                // globe.addSatellite(55.7558, 37.6173, 1.2)   // Moscow
                
                // Add pins
                globe.addPin(48.8566, 2.3522, "Paris")
                globe.addPin(34.0522, -118.2437, "LA")
                globe.addPin(-23.5505, -46.6333, "Sao Paulo")
            }
        }
        
        // Enable mouse interaction
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            
            property real lastX: 0
            property real lastY: 0
            
            onPressed: (mouse) => {
                lastX = mouse.x
                lastY = mouse.y
            }
            
            onPositionChanged: (mouse) => {
                if (pressed) {
                    // Adjust view angle based on vertical drag
                    globe.viewAngle = Math.max(-1.57, Math.min(1.57, 
                        globe.viewAngle + (mouse.y - lastY) * 0.005))
                    lastX = mouse.x
                    lastY = mouse.y
                }
            }
            
            onWheel: (wheel) => {
                // Zoom with scroll wheel
                globe.scale = Math.max(0.3, Math.min(3.0, 
                    globe.scale + wheel.angleDelta.y * 0.001))
            }
        }
        
        Repeater {
            model: globe.pinLabels
            delegate: Text {
                x: modelData.x - width/2
                y: modelData.y - height
                text: modelData.text
                color: "#FFFFFF"
                opacity: modelData.opacity
                font.family: "Inconsolata, monospace"
                font.pixelSize: 16
                font.bold: true
                style: Text.Outline
                styleColor: "#000000"
            }
        }
    }
    
    // Info overlay
    Column {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 20
        spacing: 10
        
        Label {
            text: "ENCOM GLOBE"
            font.family: "Inconsolata, monospace"
            font.pixelSize: 24
            color: "#ffcc00"
        }
        
        Label {
            text: "Drag: Adjust view angle\nScroll: Zoom"
            font.family: "Inconsolata, monospace"
            font.pixelSize: 14
            color: "#8FD8D8"
        }
    }
    
    // Controls panel
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 20
        width: 200
        height: controlsColumn.height + 20
        color: "#1a1a1a"
        radius: 5
        
        Column {
            id: controlsColumn
            anchors.centerIn: parent
            width: parent.width - 20
            spacing: 10
            
            Label {
                text: "Controls"
                font.family: "Inconsolata, monospace"
                font.pixelSize: 16
                color: "#ffcc00"
            }
            
            Label {
                text: "Scale: " + globe.scale.toFixed(2)
                font.family: "Inconsolata, monospace"
                font.pixelSize: 12
                color: "#8FD8D8"
            }
            
            Slider {
                width: parent.width
                from: 0.3
                to: 3.0
                value: globe.scale
                onValueChanged: globe.scale = value
            }
            
            Label {
                text: "Rotation: " + (globe.dayLength / 1000).toFixed(1) + "s"
                font.family: "Inconsolata, monospace"
                font.pixelSize: 12
                color: "#8FD8D8"
            }
            
            Slider {
                width: parent.width
                from: 5000
                to: 120000
                value: globe.dayLength
                onValueChanged: globe.dayLength = value
            }
        }
    }
}
