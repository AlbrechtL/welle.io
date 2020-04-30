import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3
import QtGraphicalEffects 1.0

// Import custom styles
import "texts"
import "components"

ViewBaseFrame {
    id: frame
    labelText: qsTr("Service Overview")
    Layout.maximumHeight: Units.dp(230)

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
        visible: true
        source: "qrc:/icons/welle_io_icons/20x20@2/speaker.png"

        MouseArea {
            anchors.fill: parent
            onClicked: {radioController.setVolume(0); speakerIconMutedRed.visible = true; speakerIcon.visible = false}
        }
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
        MouseArea {
            anchors.fill: parent
            onClicked: {radioController.setVolume(100); speakerIconMutedRed.visible = false; speakerIcon.visible = true}
        }
    }

    RowLayout{
        id: signalStrength
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: Units.dp(5)
        spacing: Units.dp(2)

        Accessible.name: qsTr("Signal noise ratio: " + radioController.snr )

        Rectangle{
            height: Units.dp(4)
            width: Units.dp(4)
            color: (radioController.snr > 2) ? "green" : "dimgrey"
            Accessible.ignored: true
        }
        Rectangle{
            height: Units.dp(8)
            width: Units.dp(4)
            color: (radioController.snr > 5) ? "green" : "dimgrey"
            Accessible.ignored: true
        }
        Rectangle{
            height: Units.dp(12)
            width: Units.dp(4)
            color: (radioController.snr > 8) ? "green" : "dimgrey"
            Accessible.ignored: true
        }
        Rectangle{
            height: Units.dp(16)
            width: Units.dp(4)
            color: (radioController.snr > 11) ? "green" : "dimgrey"
            Accessible.ignored: true
        }

        Rectangle{
            height: Units.dp(20)
            width: Units.dp(4)
            color: (radioController.snr > 15) ? "green" : "dimgrey"
            Accessible.ignored: true
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
                    opacity: 100
                    
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
            antennaSymbol.opacity = 100
            effect.stop()
        }
    }

    function reanchorAntenna() {
        if (!antennaSymbol.isTextTooLongForAntenna)
            antennaSymbol.state = "alignRight"
        else
            antennaSymbol.state = "alignBottom"
    }
}
