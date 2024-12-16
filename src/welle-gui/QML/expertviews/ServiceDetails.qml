/*
 *    Copyright (C) 2017 - 2021
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 
import QtQuick
import QtQuick.Layouts

// Import custom styles
import "../texts"
import "../components"


ViewBaseFrame {
    labelText: qsTr("Service Details")

    content: ColumnLayout {
        anchors.fill: parent
        Layout.margins: Units.dp(50)

        TextExpert {
            name: qsTr("Device") + ":"
            text: radioController.deviceName
        }

        TextExpert {
            name: qsTr("Current channel") + ":"
            text: radioController.channel + " (" + (radioController.frequency > 0 ? radioController.frequency/1e6 :  "N/A") + " MHz)"
        }

        TextExpert {
            name: qsTr("Frequency correction") + ":"
            text: radioController.frequencyCorrection + " Hz (" + (radioController.frequency > 0 ? radioController.frequencyCorrectionPpm.toFixed(2) : "N/A") + " ppm)"
        }

        TextExpert {
            name: qsTr("SNR") + ":"
            text: radioController.snr.toFixed(2) + " dB"
        }

        RowLayout {
            Rectangle{
                height: Units.dp(16)
                width: Units.dp(16)
                color: radioController.isSync ? "green" : "red"
            }

            TextExpert {
                name: qsTr("Frame sync")  + ":"
                text: radioController.isSync ? qsTr("OK") : qsTr("Not synced")
            }

        }

        RowLayout {
            Rectangle{
                height: Units.dp(16)
                width: Units.dp(16)
                color: radioController.isFICCRC ? "green" : "red"
            }

            TextExpert {
                name: qsTr("FIC CRC")  + ":"
                text: radioController.isFICCRC ? qsTr("OK") : qsTr("Error")
            }
        }

        RowLayout {
            Rectangle{
                height: Units.dp(16)
                width: Units.dp(16)
                color: (radioController.frameErrors === 0
                        && radioController.isSync
                        && radioController.isFICCRC) ? "green" : "red"
            }

            TextExpert {
                name: qsTr("Frame errors")  + ":"
                text: radioController.frameErrors
            }
        }

        RowLayout {
            Rectangle{
                height: Units.dp(16)
                width: Units.dp(16)
                color: (radioController.rsCorrectedErrors === 0
                        && radioController.rsUncorrectedErrors === 0)
                        ? "green" : (radioController.rsCorrectedErrors >= 0
                                     && radioController.rsUncorrectedErrors === 0) ? "yellow" : "red"
            }

            TextExpert {
                name: qsTr("RS errors")  + ":"
                text: (radioController.rsCorrectedErrors === 0
                       && radioController.rsUncorrectedErrors === 0)
                       ? qsTr("OK") : (radioController.rsCorrectedErrors >= 0
                                    && radioController.rsUncorrectedErrors === 0) ? qsTr("Corrected Error") : qsTr("Uncorrected Error")
            }
        }

        RowLayout {
            Rectangle{
                height: Units.dp(16)
                width: Units.dp(16)
                color: radioController.aacErrors === 0 ? "green" : "red"
            }

            TextExpert {
                name: qsTr("AAC errors")  + ":"
                text: radioController.aacErrors
            }
        }

        TextExpert {
            name: qsTr("Ensemble ID") + ":"
            text: "0x" + radioController.ensembleId.toString(16)
        }

        TextExpert {
            name: qsTr("DAB date and time") + ":"
            text: radioController.dateTime.toString()
        }
    }
}
