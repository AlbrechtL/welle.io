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
    property alias qQStyleTheme : qQStyleTheme.currentIndex
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
        property alias qQStyleTheme: qQStyleTheme.currentIndex
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
                WComboBoxList {
                    id: languageBox
                    textRole: 'label'
                    model: ListModel {
                        id: listModel
                        ListElement { label: "Auto"; langCode: "auto" }
                        ListElement { label: "Deutsch"; langCode: "de_DE" }
                        ListElement { label: "English (GB)"; langCode: "en_GB" }
                        ListElement { label: "Français"; langCode: "fr_FR" }
                        ListElement { label: "Magyar"; langCode: "hu_HU" }
                        ListElement { label: "Italiano"; langCode: "it_IT" }
                        ListElement { label: "Nederlands"; langCode: "nl_NL" }
                        ListElement { label: "Norsk bokmål"; langCode: "nb_NO" }
                        ListElement { label: "Polski"; langCode: "pl_PL" }
                        ListElement { label: "Ру́сский"; langCode: "ru_RU" }
                    }
                    sizeToContents: true
                    onCurrentIndexChanged: {
                        // Load appropriate settings
                        guiHelper.updateTranslator(listModel.get(currentIndex).langCode, mainWindow);
                    }
                }

                TextStandart {
                    text: qsTr("Language") == "Language" ? "Language" : qsTr("Language") + " (Language)"
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
                        id: labelSliderView
                        Layout.alignment : Qt.AlignRight
                        text: qsTr("Value: ")
                    }

                    TextStandart {
                        id: valueSliderView
                        text: radioController.gainValue.toFixed(2)
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
                onCheckedChanged: {
                    if(enableAutoSdr.checked)
                        guiHelper.openAutoDevice()
                }
            }

            WComboBoxList {
                id: deviceBox
                enabled: !enableAutoSdr.checked
                Layout.fillWidth: true

                textRole: 'trLabel'
                model: ListModel {
                    id: deviceBoxModel
                    ListElement { label: "None"; trLabel: qsTr("None"); trContext: "GlobalSettings" }
                    ListElement { label: "Airspy"; trLabel: qsTr("Airspy"); trContext: "GlobalSettings" }
                    ListElement { label: "rtl-sdr"; trLabel: qsTr("rtl-sdr"); trContext: "GlobalSettings" }
                    ListElement { label: "SoapySDR"; trLabel: qsTr("SoapySDR"); trContext: "GlobalSettings" }
                    ListElement { label: "rtl-tcp"; trLabel: qsTr("rtl-tcp"); trContext: "GlobalSettings" }
                    ListElement { label: "RAW file"; trLabel: qsTr("RAW file"); trContext: "GlobalSettings" }
                }

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

                    console.debug("Used input device: " + currentIndex)
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

        SettingSection {
            text: qsTr("Style settings")

            RowLayout {
                Layout.fillWidth: true
                WComboBoxList {
                    id: styleBox
                    sizeToContents: true
                    currentIndex: guiHelper.getIndexOfQQStyle(guiHelper.getQQStyle)
                    textRole: "label"
                    model: guiHelper.qQStyleComboModel
                    onActivated: {
                        guiHelper.saveQQStyle(currentIndex)
                        infoMessagePopup.text = qsTr("Style changed. Please restart welle.io");
                        infoMessagePopup.open();
                    }
                }
                TextStandart {
                    text: qsTr("Style. Restart to apply.")
                    Layout.fillWidth: true
                }
            }
            RowLayout {
                Layout.fillWidth: true
                WComboBoxList {
                    id: qQStyleTheme
                    sizeToContents: true
                    enabled: guiHelper.isThemableStyle(guiHelper.getQQStyle)
                    textRole: 'trLabel'
                    model: ListModel {
                        id: themeListModel
                        ListElement { label: "Light"; trLabel: qsTr("Light"); trContext: "GlobalSettings" }
                        ListElement { label: "Dark"; trLabel: qsTr("Dark"); trContext: "GlobalSettings" }
                        ListElement { label: "System"; trLabel: qsTr("System"); trContext: "GlobalSettings" }
                    }
                }
                TextStandart {
                    text: qsTr("Theme")
                    Layout.fillWidth: true
                }
                Connections {
                    target: guiHelper
                    onStyleChanged: {
                        qQStyleTheme.enabled = guiHelper.isThemableStyle(guiHelper.getQQStyle)
                    }
                }
            }
        }
    }
}
