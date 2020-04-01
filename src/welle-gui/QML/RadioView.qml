import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3
import QtGraphicalEffects 1.0
import Qt.labs.settings 1.0

// Import custom styles
import "texts"
import "components"

ViewBaseFrame {
    id: frame
    labelText: qsTr("Service Overview")
    Layout.maximumHeight: Units.dp(230)

    Settings {
        property alias volume : volumeSlider.value
    }

    TextRadioInfo {
        anchors.top: parent.top
        anchors.topMargin: Units.dp(5)
        anchors.horizontalCenter: parent.horizontalCenter
        text: radioController.ensemble.trim()
    }

    // Use 2 Images to switch between speaker & speaker_mute icon (instead of toggle button).
    // Permits use of color with org.kde.desktop style
    Image {
        id: speakerIcon
        anchors.verticalCenter: signalStrength.verticalCenter
        anchors.right: parent.right
        width: Units.dp(30)
        height: Units.dp(30)
        visible: false

        source: "qrc:/icons/welle_io_icons/20x20@2/speaker.png"

        WToolTip {
            text: qsTr("Volume")
            visible: speakerIconMouseArea.containsMouse
        }

        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Volume")
        Accessible.description: qsTr("Toggle volume slider")
        Accessible.onPressAction: speakerIconMouseArea.clicked(mouse)
    }

    MouseArea {
        id: speakerIconMouseArea
        anchors.fill: speakerIcon
        hoverEnabled: true
        onClicked: volumeSlider.visible = !volumeSlider.visible
    }
    Image {
        id: speakerIconMuted
        anchors.verticalCenter: signalStrength.verticalCenter
        anchors.right: parent.right
        width: Units.dp(30)
        height: Units.dp(30)
        visible: false

        source: "qrc:/icons/welle_io_icons/20x20@2/speaker_mute.png"
    }
    ColorOverlay {
        id: speakerIconMutedRed
        visible: false
        anchors.fill: speakerIconMuted
        source: speakerIconMuted
        color: "red"
    }

    // We don't display the "speakerIcon" item, but the "speakerIconMaskApplied"
    // item the right part of which is +/- opacified depending on the volume
    Item {
        id: hidingRect
        anchors.fill: speakerIcon
        visible: false
        Rectangle {
            anchors.right: parent.right
            color: "green" //Could be any
            width: speakerIcon.width *0.40
            height: speakerIcon.height
            opacity: 1 - volumeSlider.value
        }
    }
    OpacityMask {
        id: speakerIconMaskApplied
        anchors.fill: speakerIcon
        source: speakerIcon
        maskSource: hidingRect
        invert: true
    }

    TextRadioInfo {
        id: volumeLabel
        anchors.top: speakerIcon.bottom
        anchors.horizontalCenter: speakerIcon.horizontalCenter
        //visible: false

        font.pixelSize: Units.em(0.7)
        text: Math.round(volumeSlider.value*100) + "%"

        Accessible.description: qsTr("Volume set to %1").arg(text)
    }
    Slider {
        id: volumeSlider
        anchors.top: volumeLabel.bottom
        anchors.horizontalCenter: speakerIcon.horizontalCenter

        height: parent.height * 0.70
        orientation: Qt.Vertical
        snapMode: Slider.SnapAlways
        visible: false

        from: 0
        to: 1
        stepSize: 0.01
        value: radioController.volume

        onValueChanged: {
            setVolume(value)
            if (visible)
                volumeSliderTrigger.restart()
        }

        onVisibleChanged: {
            //volumeLabel.visible = visible
            if (visible)
                volumeSliderTrigger.restart()
            else
                volumeSliderTrigger.stop()
        }

        Connections {
            target: radioController
            onVolumeChanged: {
                volumeSlider.value = volume
            }
        }

        Timer {
            id: volumeSliderTrigger
            interval: 5000
            running: false
            repeat: false
            onTriggered: { volumeSlider.visible = false }
        }

        function setVolume(value) {
            if (volumeSlider.value != radioController.volume) {
                if (value === 0) {
                    radioController.setVolume(value)
                    speakerIconMutedRed.visible = true
                    speakerIconMaskApplied.visible = false
                } else {
                    radioController.setVolume(value)
                    speakerIconMutedRed.visible = false
                    speakerIconMaskApplied.visible = true
                }
            }
        }
    }

    RowLayout{
        id: signalStrength
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: Units.dp(5)
        spacing: Units.dp(2)

        Rectangle{
            height: Units.dp(4)
            width: Units.dp(4)
            color: (radioController.snr > 2) ? "green" : "dimgrey"
        }
        Rectangle{
            height: Units.dp(8)
            width: Units.dp(4)
            color: (radioController.snr > 5) ? "green" : "dimgrey"
        }
        Rectangle{
            height: Units.dp(12)
            width: Units.dp(4)
            color: (radioController.snr > 8) ? "green" : "dimgrey"
        }
        Rectangle{
            height: Units.dp(16)
            width: Units.dp(4)
            color: (radioController.snr > 11) ? "green" : "dimgrey"
        }

        Rectangle{
            height: Units.dp(20)
            width: Units.dp(4)
            color: (radioController.snr > 15) ? "green" : "dimgrey"
        }
    }

    Image {
        id: startStopIcon
        anchors.top: parent.top
        anchors.right: speakerIcon.left
        anchors.rightMargin: Units.dp(5)
        anchors.verticalCenter: signalStrength.verticalCenter

        height: Units.dp(15)
        fillMode: Image.PreserveAspectFit

        Accessible.role: Accessible.Button
        Accessible.name: radioController.isPlaying ? qsTr("Stop") : qsTr("Play")
        Accessible.description: radioController.isPlaying ? qsTr("Stop playback") : qsTr("Start playback")
        Accessible.onPressAction: startStopIconMouseArea.clicked(mouse)

        WToolTip {
            text: radioController.isPlaying ? qsTr("Stop") : qsTr("Play")
            visible: startStopIconMouseArea.containsMouse
        }

        Component.onCompleted: { startStopIcon.setStartPlayIcon() }

        MouseArea {
            id: startStopIconMouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                if (radioController.isPlaying) {
                    radioController.stop();
                } else {
                    var channel = radioController.lastChannel[1]
                    var sidHex = radioController.lastChannel[0]
                    var sidDec = parseInt(sidHex,16);
                    var stationName = stationList.getStationName(sidDec, channel)
                    //console.debug("stationName: " + stationName + " channel: " + channel + " sidHex: "+ sidHex)
                    if (!channel || !sidHex || !stationName) {
                        infoMessagePopup.text = qsTr("Last played station not found.\nSelect a station to start playback.");
                        infoMessagePopup.open();
                    } else {
                        radioController.play(channel, stationName, sidDec)
                    }
                }
            }
        }

        Connections {
            target: radioController
            onIsPlayingChanged: {
                startStopIcon.setStartPlayIcon()
            }
        }

        function setStartPlayIcon() {
            if (radioController.isPlaying) {
                startStopIcon.source = "qrc:/icons/welle_io_icons/20x20/stop.png"
            } else {
                startStopIcon.source = "qrc:/icons/welle_io_icons/20x20/play.png"
            }
        }
    }


    ColumnLayout {
        anchors.centerIn: parent

        /* Station Name */
        RowLayout {
            Layout.alignment: Qt.AlignHCenter

            TextRadioStation {
                id:textRadioStation
                Layout.margins: Units.dp(10)
                Layout.maximumWidth: parent.parent.parent.width
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                text: radioController.title.trim()

                // Use an Item to display icons as Image
                Item {
                    property bool isSignal: false
                    id: antennaSymbol
                    
                    anchors.leftMargin: Units.dp(10)
                    implicitWidth: antennaIcon.width
                    implicitHeight: implicitWidth
                    
                    property bool isTextTooLongForAntenna: {
                        var maxWidth = parent.parent.parent.parent.width;
                        var textAndAntennaWidth = parent.width + implicitWidth + Units.dp(30) + anchors.leftMargin;
                        return ( textAndAntennaWidth > maxWidth )
                    }
                    
                    visible: opacity == 0 ? false : true
                    opacity: 0
                    
                    Connections {
                        target: frame
                        onWidthChanged: { reanchorAntenna() }
                    }
                    
                    Connections {
                        target: textRadioStation
                        onTextChanged: { reanchorAntenna() }
                    }
                    
                    states: [
                    State {
                        name: "alignRight"
                        AnchorChanges {
                            target: antennaSymbol
                            anchors.left: textRadioStation.right
                            anchors.verticalCenter: textRadioStation.verticalCenter
                            anchors.top: undefined
                            anchors.horizontalCenter: undefined
                        }
                    },
                    State {
                        name: "alignBottom"
                        AnchorChanges {
                            target: antennaSymbol
                            anchors.left: undefined
                            anchors.verticalCenter: undefined
                            anchors.top: textRadioStation.bottom
                            anchors.horizontalCenter: textRadioStation.horizontalCenter
                        }
                    }
                    ]
                    
                    Image {
                        id: antennaIcon
                        width: Units.dp(30)
                        height: Units.dp(30)
                        visible: false
                        source: "qrc:/icons/welle_io_icons/20x20@2/antenna.png"
                    }
                    
                    Image {
                        id: antennaIconNoSignal
                        width: antennaIcon.width
                        height: antennaIcon.height
                        visible: false
                        source: "qrc:/icons/welle_io_icons/20x20@2/antenna_no_signal.png"
                    }
                    
                    ColorOverlay {
                        id: antennaIconNoSignalRed
                        visible: true
                        anchors.fill: antennaIconNoSignal
                        source: antennaIconNoSignal
                        color: "red"
                    }
                    
                    Connections {
                        target: antennaSymbol
                        onIsSignalChanged: { 
                            if (!radioController.isPlaying) {
                                hideAntenna()
                                return
                            }
                            if (antennaSymbol.isSignal) {
                                antennaIconNoSignalRed.visible = false; 
                                antennaIcon.visible = true;
                            } else {
                                antennaIconNoSignalRed.visible = true; 
                                antennaIcon.visible = false;
                            }
                        }
                    }
                    
                    NumberAnimation on opacity {
                        id: effect
                        to: 0;
                        duration: 6000;
                        running: false
                    }

                    Connections {
                        target: radioController
                        onIsFICCRCChanged: {
                            if(radioController.isFICCRC)
                                __setIsSignal(true)
                            else
                                __setIsSignal(false)
                        }

                        onIsSyncChanged: {
                            if(radioController.isSync)
                                __setIsSignal(true)
                            else
                                __setIsSignal(false)
                        }

                        onIsPlayingChanged: {
                            if (!radioController.isPlaying)
                                hideAntenna()
                        }
                    }
                }
            }
        }

        /* Station Text */
        TextRadioInfo {
            Layout.alignment: Qt.AlignHCenter
            Layout.margins: Units.dp(10)
            Layout.maximumWidth: parent.parent.width
            width: frame.width
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            text: radioController.text.trim()
        }
    }

    RowLayout{
        id: stationInfo
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Units.dp(5)
        width: parent.width

        TextRadioInfo {
            id: stationType
            visible: stationInfo.visible
            Layout.alignment: Qt.AlignLeft
            Layout.leftMargin: Units.dp(5)
            verticalAlignment: Text.AlignBottom
            text: qsTranslate("DABConstants", radioController.stationType)
        }

        TextRadioInfo {
            visible: stationInfo.visible
            Layout.alignment: Qt.AlignRight
            Layout.rightMargin: Units.dp(5)
            verticalAlignment: Text.AlignBottom
            Layout.maximumWidth: parent.parent.width - stationType.width
            fontSizeMode: Text.Fit
            minimumPixelSize: 8;
            text: (radioController.isDAB ? "DAB" : "DAB+")
                + " " + radioController.audioMode
        }
    }

    Component.onCompleted: {
        if(radioController.isFICCRC &&
                radioController.isSync)
            __setIsSignal(true)
    }

    function __setIsSignal(value) {
        if(value) {
            antennaSymbol.isSignal = true
            effect.restart()
        }
        else {
            antennaSymbol.isSignal = false
            antennaSymbol.opacity = 1.0
            effect.stop()
        }
    }

    function reanchorAntenna() {
        if (!antennaSymbol.isTextTooLongForAntenna)
            antennaSymbol.state = "alignRight"
        else
            antennaSymbol.state = "alignBottom"
    }

    function hideAntenna() {
        antennaIconNoSignalRed.visible = false;
        antennaIcon.visible = false;
    }
}
