import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

// Import custom styles
import "../texts"
import "../components"

Item {
    id: settingsPage

    property alias enableFullScreenState : enableFullScreen.checked
    property bool isLoaded: false

    anchors.fill: parent
    implicitHeight: layout.implicitHeight

    Settings {
        property alias device: deviceBox.currentIndex
        property alias enableAGCState : enableAGC.checked
        property alias enableFullScreenState : settingsPage.enableFullScreenState
        property alias manualGainState : manualGain.value
        property alias manualGainValue: valueSliderView.text
        property alias enableAutoSdr : enableAutoSdr.checked
        property alias languageValue : languageBox.currentIndex
    }

    Component.onCompleted: {
        console.debug("Apply settings initially")
        radioController.setGain(manualGain.value)
        radioController.setAGC(enableAGC.checked)
    }

    Connections{
        target: radioController

        onGainCountChanged: manualGain.to = radioController.gainCount
    }

    Connections {
        target: guiHelper

        onNewDeviceId: {
            switch(deviceId) {
            case 0: deviceBox.currentIndex = 0; break; // UNKNOWN
            case 1: deviceBox.currentIndex = 0; break; // NULLDEVICE
            case 2: deviceBox.currentIndex = 1; break; // AIRSPY
            case 3: deviceBox.currentIndex = 5; break; // RAWFILE
            case 4: deviceBox.currentIndex = 2; break; // RTL_SDR
            case 5: deviceBox.currentIndex = 4; break; // RTL_TCP
            case 6: deviceBox.currentIndex = 3; break; // SOAPYSDR
            default: deviceBox.currentIndex = 0;
            }
        }
    }

    ColumnLayout{
        id: layout
        anchors.fill: parent
        spacing: Units.dp(20)

        SettingSection {
            id: settingsFrame
            isNotFirst: false
            text: qsTr("Global settings")

            WSwitch {
                id: enableFullScreen
                text: qsTr("Full screen mode")
                Layout.fillWidth: true
                checked: false
            }

            RowLayout {
                Layout.fillWidth: true
                WComboBox {
                    id: languageBox
                    model: [ "Auto", "Dutch", "English", "German", "Polish", "Norwegian"];
                    onCurrentIndexChanged: {
                        // Load appropriate settings
                        switch(currentIndex) {
                        case 1: guiHelper.addTranslator("nl_NL", this); break
                        case 2: guiHelper.addTranslator("en_GB", this); break
                        case 3: guiHelper.addTranslator("de_DE", this); break
                        case 4: guiHelper.addTranslator("pl_PL", this); break
                        case 5: guiHelper.addTranslator("nb_NO", this); break
                        default: guiHelper.addTranslator("auto", this); break
                        }
                    }
                }

                TextStandart {
                    text: qsTr("Language")
                    Layout.fillWidth: true
                }
            }
        }

        SettingSection {
            text: qsTr("Global receiver settings")

            WSwitch {
                id: enableAGC
                text: qsTr("Automatic RF gain")
                Layout.fillWidth: true
                checked: true
                onClicked: {
                    radioController.setAGC(checked)

                    if(checked == false)
                        radioController.setGain(manualGain.value)
                }
            }

            ColumnLayout {
                spacing: Units.dp(10)
                opacity: enabled ? 1 : 0.5
                enabled: !enableAGC.checked

                RowLayout {
                    TextStandart {
                        id: nameSliderView
                        Layout.fillWidth: true
                        text: qsTr("Manual gain")
                    }

                    TextStandart {
                        id: valueSliderView
                        text: qsTr("Value: ") + radioController.gainValue.toFixed(2)
                    }
                }

                Slider {
                    id: manualGain
                    from: 0
                    to: 100
                    stepSize: 1
                    Layout.fillWidth: true

                    onValueChanged: {
                        if(enableAGC.checked == false)
                            radioController.setGain(value)
                    }
                }
            }

            WSwitch {
                id: enableAutoSdr
                text: qsTr("Auto detect")
                Layout.fillWidth: true
                checked: true
            }

            WComboBox {
                id: deviceBox
                enabled: !enableAutoSdr.checked
                Layout.fillWidth: true
                model: [ "None", "Airspy", "rtl-sdr", "SoapySDR", "rtl-tcp", "RAW file"];
                onCurrentIndexChanged: {
                    // Load appropriate settings
                    switch(currentIndex) {
                    case 1: sdrSpecificSettings.source = "qrc:/QML/settingpages/AirspySettings.qml"; break
                    case 2: sdrSpecificSettings.source = "qrc:/QML/settingpages/RTLSDRSettings.qml"; break
                    case 3: sdrSpecificSettings.source = "qrc:/QML/settingpages/SoapySDRSettings.qml"; break
                    case 4: sdrSpecificSettings.source = "qrc:/QML/settingpages/RTLTCPSettings.qml"; break
                    case 5: sdrSpecificSettings.source = "qrc:/QML/settingpages/RawFileSettings.qml"; break
                    default: sdrSpecificSettings.source = "qrc:/QML/settingpages/NullSettings.qml"; break
                    }
                }
            }

            Component.onCompleted: {
                if(enableAutoSdr.checked)
                    guiHelper.openAutoDevice()

                sdrSpecificSettings.item.initDevice(enableAutoSdr.checked)
                isLoaded = true
            }
        }

        Loader {
            id: sdrSpecificSettings
            Layout.fillWidth: true

            onLoaded: {
                if(isLoaded)
                    item.initDevice(enableAutoSdr.checked)
            }
        }
    }
}
