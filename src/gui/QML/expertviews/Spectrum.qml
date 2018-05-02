import QtQuick 2.2
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1

// Import custom styles
import "../texts"
import "../components"

Item {
    height: layout.height
    width: layout.width

    ColumnLayout {
        id: layout

        RowLayout {
            SpectrumView {
                id: plot
                Layout.preferredWidth: Units.dp(400)
                Layout.preferredHeight: Units.dp(400)
            }

            ColumnLayout {
                anchors.right: parent.right
                anchors.top: parent.top
                Button {
                    id: buttonSpec
                    text: qsTr("Spectrum")
                    onClicked: {
                        cppGUI.setPlotType(0);
                        plot.plotType = 0;
                    }
                }
                Button {
                    id: buttonCIR
                    text: qsTr("CIR")
                    onClicked: {
                        cppGUI.setPlotType(1);
                        plot.plotType = 1;
                    }
                }
                Button {
                    id: buttonQPSK
                    text: qsTr("QPSK")
                    onClicked: {
                        cppGUI.setPlotType(2);
                        plot.plotType = 2;
                    }
                }
                Button {
                    id: buttonNull
                    text: qsTr("Null")
                    onClicked: {
                        cppGUI.setPlotType(3);
                        plot.plotType = 3;
                    }
                }
            }
        }
    }
}
