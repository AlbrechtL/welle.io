import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2

// Import custom styles
import "../texts"
import "../components"

SettingSection {
    text: qsTr("rtl-tcp settings")

    RowLayout {
        spacing: Units.dp(20)
        ColumnLayout {
            TextStandart {
                text: qsTr("IP address")
                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout {
                id:layout
                height: Units.dp(120)
                spacing: Units.dp(5)

                SettingTumbler {
                    id: ipAddress1
                    currentIndex: 192
                }
                SettingTumbler {
                    id: ipAddress2
                    currentIndex: 168
                }
                SettingTumbler {
                    id: ipAddress3
                    currentIndex: 1
                }
                SettingTumbler {
                    id: ipAddress4
                    currentIndex: 10
                }
            }
        }

        ColumnLayout {
            TextStandart {
                text: qsTr("IP port")
            }

            SettingTumbler {
                id: ipPort
                model: 65536
                currentIndex: 1234
            }
        }

        Button {
            text: qsTr("Apply")
            Layout.fillWidth: true
            onClicked: {
                var ipAdress =
                        ipAddress1.currentIndex + "."
                        + ipAddress2.currentIndex + "."
                        + ipAddress3.currentIndex + "."
                        + ipAddress4.currentIndex
                radioController.openDevice(4, true, ipAdress, ipPort.currentIndex)
            }
        }
    }
}

