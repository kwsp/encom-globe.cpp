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
        
        startupDelay: 2000 // 2 second pause before animations begin
        dayLength: 28000  // ms per rotation
        scale: 1.0        // zoom factor
        viewAngle: 0.1    // tilt in radians
        baseColor: "#ffcc00"
        pinColor: "#00eeee"
        markerColor: "#ffcc00"
        satelliteColor: "#ff0000"
        introLinesColor: "#8FD8D8"
        markerSize: 0.5   // marker scale multiplier
        pinHeadSize: 0.2  // pin head scale multiplier
        showLabels: false
        introDuration: 2000 // ms
        
        // Add some test satellites after intro
        Timer {
            interval: globe.startupDelay + globe.introDuration + 500
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
                globe.addPin(-23.5505, -46.6333, "Sao Paulo")
                
                // Add markers
                globe.addMarker(40.7128, -74.0060, "NYC", false)
                globe.addMarker(34.0522, -118.2437, "LA", true)
                globe.addMarker(35.6762, 139.6503, "Tokyo", true)
                globe.addMarker(-33.8688, 151.2093, "Sydney", true)
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
                    // Adjust rotation offset based on horizontal drag
                    let dx = mouse.x - lastX
                    globe.rotationOffset += dx * 0.005
                    
                    // Adjust view angle based on vertical drag
                    let dy = mouse.y - lastY
                    globe.viewAngle = Math.max(-1.57, Math.min(1.57, 
                        globe.viewAngle + dy * 0.005))
                    
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
                visible: globe.showLabels
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
            
            Label {
                text: "Pin Head Size: " + globe.pinHeadSize.toFixed(2)
                font.family: "Inconsolata, monospace"
                font.pixelSize: 12
                color: "#8FD8D8"
            }
            
            Slider {
                width: parent.width
                from: 0.1
                to: 3.0
                value: globe.pinHeadSize
                onValueChanged: globe.pinHeadSize = value
            }

            Label {
                text: "Marker Size: " + globe.markerSize.toFixed(2)
                font.family: "Inconsolata, monospace"
                font.pixelSize: 12
                color: "#8FD8D8"
            }
            
            Slider {
                width: parent.width
                from: 0.1
                to: 2.0
                value: globe.markerSize
                onValueChanged: globe.markerSize = value
            }
            
            Label {
                text: "Theme"
                font.family: "Inconsolata, monospace"
                font.pixelSize: 14
                color: "#ffcc00"
            }
            
            Row {
                spacing: 5
                property var colors: ["#ffcc00", "#00eeee", "#ff4444", "#44ff44", "#ffffff"]
                Repeater {
                    model: parent.colors
                    delegate: Rectangle {
                        width: 30; height: 30
                        color: modelData
                        border.color: globe.baseColor === modelData ? "white" : "transparent"
                        border.width: 2
                        radius: 3
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                globe.baseColor = modelData
                                globe.markerColor = modelData
                            }
                        }
                    }
                }
            }

            Button {
                text: "Drop random pin"
                width: parent.width
                onClicked: {
                    var lat = Math.random() * 180 - 90
                    var lon = Math.random() * 360 - 180
                    globe.addPin(lat, lon, "User Pin")
                }
            }
            
            CheckBox {
                text: "Show Labels"
                checked: globe.showLabels
                onCheckedChanged: globe.showLabels = checked
                palette.windowText: "#ffcc00"
            }
        }
    }
}
