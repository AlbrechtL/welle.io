import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.1


// Import custom styles
import "../texts"
import "../components"

ViewBaseFrame {
    labelText: qsTr("I/Q RAW Recorder (experimental)")

    property bool isStart: false
    property int ringeBufferSize: 0

    content:  ColumnLayout {
        RowLayout {
            TextStandart {
                text: qsTr("Ring buffer length [s]")
            }

            WTumbler {
                id: ringeBufferSetting
                model: [5, 10, 60, 120, 240]


                onCurrentIndexChanged: {
                     ringeBufferSize = parseInt(ringeBufferSetting.currentItem.text) * 2048 * 1024
                }
            }

            WButton {
                text: isStart ? qsTr("Save ring buffer") : qsTr("Init")

                onPressed: {
                    if(!isStart)
                        radioController.initRecorder(ringeBufferSize)
                    else
                        radioController.triggerRecorder("")

                    isStart = !isStart
                }
            }
        }

        TextStandart {
            text: qsTr("Ring buffer size (roughly): ") + (ringeBufferSize / 1000000 * 2).toFixed(0) + " MB"
        }
    }
}
