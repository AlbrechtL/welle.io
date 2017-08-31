import QtQuick 2.6
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.3
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0
import "style"

Item {
    id:__dablayout
    clip: true
    property alias settingsVisible: item2.visible
    property alias infoPageVisible: infoPageLoader.visible
    property alias enableFullScreenState : enableFullScreen.checked
    property alias enableExpertModeState : enableExpertMode.checked
    property alias enableAGCState : enableAGC.checked
    property alias manualGainState : manualGain.currentValue
    property alias is3D : enable3D.checked

    ListModel {
        id: colors
        ListElement {
            color:"#F44336"
        }
        ListElement {
            color:"#9C27B0"
        }
        ListElement {
            color:"#3F51B5"
        }
        ListElement {
            color:"#03A9F4"
        }
        ListElement {
            color:"#009688"
        }
        ListElement {
            color:"#8BC34A"
        }
        ListElement {
            color:"#FFEB3B"
        }
        ListElement {
            color:"#FF9800"
        }
        ListElement {
            color:"#FF5722"
        }
        ListElement {
            color:"#9E9E9E"
        }
    }

    ListModel {
        id: dabModel
        function fill(elements){
            elements.forEach(function(item){
                append(item);
            });
        }
    }

    Item {
        id: dragger
        width: 1
        height: 1
        visible: false
        property int scrollStart:0
        onXChanged: {
            if(x<=0){
                scrollStart = x+__dablayout.width;
            } else if(x>=__dablayout.width){
                scrollStart = x-__dablayout.width;
            }

            if(scrollStart - x >= 100){
                __dablayout.state = "open";
                channelCloseTimer.restart();
                colors.move(colors.count-1,0,1);
                dabModel.move(dabModel.count-1,0,1);
                scrollStart = x;
            } else if (scrollStart - x <= -100) {
                __dablayout.state = "open";
                channelCloseTimer.restart();
                colors.move(0,colors.count-1,1);
                dabModel.append(dabModel.get(0));
                dabModel.remove(0);
                scrollStart = x;
            }
        }
    }



    Item {
        id:channels
        height: parent.height*0.4
        anchors.bottom: parent.bottom
        anchors.bottomMargin: settingsVisible?8:parent.height/2-height/2
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 8
        Item {
            id: channelsWrapper
            height: parent.height
            width:height
            Repeater {
                id: rep1
                model:dabModel
                Item {
                    property int itemsIndex : index
                    id: item1
                    clip: true
                    z: -index
                    y:(parent.height/2)-height/2
                    x: __dablayout.state == "open"?(channelsWrapper.width)/rep1.count * index:0
                    height:width
                    width:{
                        if(rep1.count == 1){
                            return parent.height
                        } else {
                            var minp = 1;
                            var maxp = rep1.count;
                            var minv = Math.log(channelsWrapper.height);
                            var maxv = Math.log(20);
                            var scale = (maxv-minv) / (maxp-minp);
                            return Math.exp(minv + scale*(index+1-minp));
                        }
                    }
                    Rectangle {
                        id: rectangle
                        color:index<0?colors.get(0).color:colors.get(index % 10).color
                        border.width: 1
                        border.color: "#ffffff"
                        anchors.fill: parent

                        Text {
                            id: text1
                            color: "#ffffff"
                            text: stationName
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideMiddle
                            anchors.left: parent.left
                            anchors.leftMargin: 0
                            anchors.right: parent.right
                            anchors.rightMargin: 0
                            anchors.verticalCenter: parent.verticalCenter
                            font.pixelSize: parent.height * 0.1
                        }
                    }
                    Behavior on x { PropertyAnimation {easing.type: Easing.Linear; duration: 250} }
                    Behavior on width { PropertyAnimation {easing.type: Easing.Linear; duration: 250} }
                }

            }
            Behavior on width { PropertyAnimation {easing.type: Easing.InOutBounce; duration: 250} }
            Timer {
                id:channelCloseTimer
                interval: 2000; running: false; repeat: false
                onTriggered:{
                    __dablayout.state = ""
                    cppGUI.channelClick(dabModel.get(0).stationName, dabModel.get(0).channelName)
                }
            }
        }

        Item {
            id: controls
            y: 0
            width: parent.width - parent.height
            height: parent.height
            anchors.left: channelsWrapper.right
            anchors.leftMargin: 0
            anchors.verticalCenter: parent.verticalCenter

            Behavior on height { PropertyAnimation {easing.type: Easing.Linear; duration: 250} }
            Item {
                id: signal_info
                height: 30
                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.right: parent.right
                anchors.rightMargin: 0

                RowLayout{
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 8
                    anchors.left: parent.left
                    spacing: 2

                    Rectangle{
                        id: signalBar1
                        height: 4
                        width: 4
                        color: (cppRadioController.SNR > 2) ? "green" : "grey"
                    }
                    Rectangle{
                        id: signalBar2
                        height: 8
                        width: 4
                        color: (cppRadioController.SNR > 5) ? "green" : "grey"
                    }
                    Rectangle{
                        id: signalBar3
                        height: 12
                        width: 4
                        color: (cppRadioController.SNR > 8) ? "green" : "grey"
                    }
                    Rectangle{
                        id: signalBar4
                        height: 16
                        width: 4
                        color: (cppRadioController.SNR > 11) ? "green" : "grey"
                    }

                    Rectangle{
                        id: signalBar5
                        height: 20
                        width: 4
                        color: (cppRadioController.SNR > 15) ? "green" : "grey"
                    }

                }

                Text {
                    id: ensembleText
                    color: "#ffffff"
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: 22
                }

                RowLayout{
                    id: flags
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 5
                    spacing: 2

                    Rectangle{
                        id: sync
                        height: 16
                        width: 16
                        color: cppRadioController.isSync ? "green" : "red"
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                    }
                    Rectangle{
                        id: fic
                        height: 16
                        width: 16
                        color: cppRadioController.isFICCRC ? "green" : "red"
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                    }
                    Rectangle{
                        id: frameSucess
                        height: 16
                        width: 16
                        color: (cppRadioController.FrameErrors === 0
                                && cppRadioController.isSync
                                && cppRadioController.isFICCRC) ? "green" : "red"
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                    }
                }
            }

            Text {
                id: stationTitle
                color: "#ffffff"
                text: qsTr("")
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                verticalAlignment: Text.AlignVCenter
                anchors.bottom: stationText.top
                anchors.bottomMargin: 0
                anchors.top: signal_info.bottom
                anchors.topMargin: 0
                horizontalAlignment: Text.AlignHCenter
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.right: parent.right
                anchors.rightMargin: 0
                font.pixelSize: 41

                Timer {
                    id:scanningTimer
                    interval: 500; running: false; repeat: true
                    onTriggered:{
                        switch(stationTitle.text){
                        case "Scanning":
                            stationTitle.text = "Scanning."
                            break;
                        case "Scanning.":
                            stationTitle.text = "Scanning.."
                            break;
                        case "Scanning..":
                            stationTitle.text = "Scanning..."
                            break;
                        default :
                            stationTitle.text = "Scanning"
                            break;
                        }
                    }
                }
            }

            Text {
                id: stationText
                color: "#ffffff"
                text: qsTr("")
                verticalAlignment: Text.AlignTop
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                anchors.topMargin: parent.height/2
                anchors.top: parent.top
                anchors.bottom: radio_info.top
                anchors.bottomMargin: 0
                anchors.right: parent.right
                anchors.rightMargin: 0
                anchors.left: parent.left
                anchors.leftMargin: 0
                font.pixelSize: 18
            }

            Item {
                id: radio_info
                visible: false
                height: 30
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0

                Text {
                    id: stationTypeText
                    visible: radio_info.visible
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignLeft
                    anchors.left: parent.left
                    anchors.right: stationDetails.left
                    anchors.leftMargin: 0
                    anchors.rightMargin: 0
                    font.pixelSize: 22
                }

                Text {
                    id: languageTypeText
                    visible: radio_info.visible
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: 22
                }

                Text {
                    id: stationDetails
                    visible: radio_info.visible
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignRight
                    anchors.left: stationDetails.right
                    anchors.right: parent.right
                    anchors.leftMargin: 0
                    anchors.rightMargin: 0
                    font.pixelSize: 22
                    text: cppRadioController.BitRate + " kbps, "
                          + (cppRadioController.isStereo ? "Stereo" : "Mono")
                          + (cppRadioController.isDAB ? ", DAB" : ", DAB+")
                }

                anchors.leftMargin: 5
                anchors.right: parent.right
                anchors.rightMargin: 5
                anchors.left: parent.left
            }
        }
    }

    MouseArea {
        id: mouseArea
        enabled: true
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton;
        onPressed: {
            dragger.scrollStart = mouse.x
            dragger.x = mouse.x
        }
        drag.target: dragger
        drag.axis: Drag.XAxis

        onWheel: {
            if(rep1.count == 1){
                var tmp = parent.height
            } else {
                if(wheel.angleDelta.y > 0){
                    __dablayout.state = "open";
                    channelCloseTimer.restart();
                    colors.move(colors.count-1,0,1);
                    dabModel.move(dabModel.count-1,0,1);
                } else if (wheel.angleDelta.y < 0) {
                    __dablayout.state = "open";
                    channelCloseTimer.restart();
                    colors.move(0,colors.count-1,1);
                    dabModel.append(dabModel.get(0));
                    dabModel.remove(0);
                }
            }
        }
    }

    GridLayout {
        id: item2
        visible: false
        anchors.bottom: channels.top
        anchors.bottomMargin: 8
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 8
        flow: GridLayout.TopToBottom
        rows: 5
        columns: 2

        Item {
            id: scanWrapper
            height: 40
            Layout.columnSpan: 2
            Layout.fillHeight: true
            Layout.fillWidth: true

            Button {
                id: scanButton
                text: "Start scanning"
                anchors.verticalCenter: parent.verticalCenter
                onClicked: {
                    switch(text){
                    case"Start scanning":
                        text = "Stop scanning"
                        scanningTimer.start();
                        cppGUI.startChannelScanClick();
                        break;
                    case"Stop scanning":
                        text = "Start scanning"
                        scanningTimer.stop();
                        cppGUI.stopChannelScanClick();
                        break;
                    }
                }
            }
            ProgressBar{
                id: channelScanProgressBar
                property alias text: textView.text
                height: 28
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 0
                anchors.left: scanButton.right
                anchors.leftMargin: 0
                minimumValue: 0
                maximumValue: 54 // 54 channels

                style:ProgressBarStyle {
                    panel: Rectangle {
                        implicitHeight: 25
                        implicitWidth: 400
                        color: "#444"
                        opacity: 0.8
                        Rectangle {
                            antialiasing: true
                            radius: 1
                            color: "#468bb7"
                            height: parent.height
                            width: parent.width * control.value / control.maximumValue
                        }
                    }
                }
                implicitWidth: parent.width
                Text {
                    id: textView
                    font.pixelSize: 18
                    color: "#ffffff"
                    anchors.centerIn: parent
                    text: qsTr("Found channels") + ": 0"
                }
            }
        }

        TouchSwitch {
            id: enableAGC
            name: qsTr("Automatic RF gain")
            Layout.fillHeight: true
            Layout.fillWidth: true
            objectName: "enableAGC"
            checked: true
            onChanged: {
                cppGUI.inputEnableAGCChanged(valueChecked)

                if(valueChecked == false)
                    cppGUI.inputGainChanged(manualGain.currentValue)
            }
        }

        Item {
            Layout.rowSpan: 2
            Layout.fillHeight: true
            Layout.fillWidth: true
            TouchSlider {
                id: manualGain
                name: qsTr("Manual gain")
                anchors.fill: parent
                visible: !enableAGC.checked
                enabled: !enableAGC.checked
                maximumValue: cppGUI.gainCount?cppGUI.gainCount:0
                showCurrentValue: qsTr("Value: ") + cppGUI.currentGainValue.toFixed(2)
                onValueChanged: {
                    if(enableAGC.checked == false)
                        cppGUI.inputGainChanged(valueGain)
                }
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                //Layout.preferredWidth: parent.width

                TouchButton {
                    id: clearListButton
                    text: qsTr("Clear station list")
                    Layout.preferredWidth: Units.dp(150)
                    Layout.alignment: Qt.AlignLeft
                    onClicked: cppGUI.clearStationList()
                }

                TouchComboBox {
                    id: manualChannelBox
                    enabled: true
                    model: ["5A", "5B", "5C", "5D",
                        "6A", "6B", "6C", "6D",
                        "7A", "7B", "7C", "7D",
                        "8A", "8B", "8C", "8D",
                        "9A", "9B", "9C", "9D",
                        "10A", "10B", "10C", "10D",
                        "11A", "11B", "11C", "11D",
                        "12A", "12B", "12C", "12D",
                        "13A", "13B", "13C", "13D", "13E", "13F",
                        "LA", "LB", "LC", "LD",
                        "LE", "LF", "LG", "LH",
                        "LI", "LJ", "LK", "LL",
                        "LM", "LN", "LO", "LP"]

                    Layout.preferredHeight: Units.dp(25)
                    Layout.preferredWidth: Units.dp(130)
                    Layout.alignment: Qt.AlignRight
                    onActivated: {
                        cppGUI.setManualChannel(model[index])
                    }
                }
            }
        }


        TouchSwitch {
            id: enableFullScreen
            name: qsTr("Full screen mode")
            Layout.fillWidth: true
            Layout.fillHeight: true
            objectName: "enableFullScreen"
            checked: false
        }        

        TouchSwitch {
            id: enable3D
            name: qsTr("Channel list layout")
            Layout.fillWidth: true
            Layout.fillHeight: true
            objectName: "enable3D"
            checked: false
            onText: "3D"
            offText: "2D"
        }

        TouchSwitch {
            id: enableExpertMode
            name: qsTr("Expert mode")
            Layout.fillWidth: true
            Layout.fillHeight: true
            objectName: "enableExpertMode"
            checked: false
        }
        Item {
            id: item3
            Layout.fillHeight: true
            Layout.fillWidth: true
            TouchButton {
                id: exitAppButton
                width: 100
                height: 25
                text: qsTr("Exit welle.io")
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: Qt.quit()
            }
        }
    }

    Loader {
        id:infoPageLoader
        anchors.fill: parent
        visible:false
        active:visible
        sourceComponent:InfoPage{

        }
    }

    Component.onCompleted:{
        dabModel.fill(cppGUI.stationModel);
    }

    Connections{
        target: cppGUI

        onSetGUIData:{
            // Title
            if(!scanningTimer.running)
                stationTitle.text = GUIData.Title

            // Text
            stationText.text = GUIData.Text

            // Ensemble
            ensembleText.text = GUIData.Ensemble

            // Station info
            radio_info.visible = (GUIData.Status === 2 || GUIData.Status === 3)

            // Station type
            stationTypeText.text = GUIData.StationType

            // Language type
            languageTypeText.text = GUIData.LanguageType

            // Channel
            var channelIndex = manualChannelBox.find(GUIData.Channel)
            if (channelIndex !== -1)
                manualChannelBox.currentIndex = channelIndex
        }

        onStationModelChanged: {
            dabModel.clear();
            dabModel.fill(cppGUI.stationModel);
        }
        onChannelScanStopped:{
            scanButton.text = "Start scanning"
            scanningTimer.stop();
            scanText.text=""
        }

        onChannelScanProgress:{
            channelScanProgressBar.value = progress
        }

        onFoundChannelCount:{
            channelScanProgressBar.text = qsTr("Found channels") + ": " + channelCount;
        }
    }

    states: [
        State {
            name: "open"

            PropertyChanges {
                target: channelsWrapper
                width: parent.width*1.2
            }
        }
    ]

}
