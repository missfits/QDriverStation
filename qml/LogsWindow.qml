/*
 * Copyright (c) 2015-2016 WinT 3794 <http://wint3794.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

import QtQuick 2.0
import QtQuick.Window 2.0
import QtQuick.Layouts 1.0
import Qt.labs.settings 1.0

import "widgets"
import "globals.js" as Globals

Window {
    id: powerDashboard

    property double power: 0
    property double voltage: 0
    property double maxPower: 0
    property double maxVoltage: 0
    property string powerString: Globals.invalidStr
    property string voltageString: Globals.invalidStr

    x: Globals.scale (300)
    y: Globals.scale (200)
    title: qsTr ("Power Dashboard")
    color: Globals.Colors.Background

    minimumWidth: Globals.scale (720)
    maximumWidth: Globals.scale (720)
    minimumHeight: Globals.scale (360)
    maximumHeight: Globals.scale (360)

    Component.onCompleted: {
        if (showDashboardOnInit.checked)
            show()
    }

    Connections {
        target: DriverStation
        onProtocolChanged: {
            powerString = Globals.invalidStr
            voltageString = Globals.invalidStr

            maxVoltage = DriverStation.nominalBatteryVoltage()
            maxPower = DriverStation.nominalBatteryAmperage() * maxVoltage

            maxPower = Math.round (maxPower * 100) / 100
            maxVoltage = Math.round (maxVoltage * 100) / 100

            canvas.requestPaint()
        }

        onVoltageChanged: {
            voltage = voltage
            power = voltage * DriverStation.nominalBatteryAmperage()

            power = Math.round (power * 100) / 100
            voltage = Math.round (voltage * 100) / 100

            powerString = power + " W"
            voltageString = voltageString

            canvas.requestPaint()
        }
    }

    Timer {
        id: timer
        repeat: true
        interval: 20
        Component.onCompleted: start()

        property int pos

        onTriggered: {
            pos += 1
            canvas.requestPaint()
        }
    }

    Settings {
        category: "Power Dashboard"
        property alias x: powerDashboard.x
        property alias y: powerDashboard.y
        property alias showOnInt: showDashboardOnInit.checked
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Globals.spacing

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            RowLayout {
                anchors.fill: parent
                spacing: Globals.spacing
                anchors.margins: Globals.spacing

                GridLayout {
                    columns: 2
                    Layout.fillHeight: true
                    columnSpacing: Globals.spacing
                    rowSpacing: Globals.spacing / 2

                    Label {
                        text: qsTr ("Voltage") + ": "
                    } Label {
                        text: voltageString
                    } Label {
                        text: qsTr ("Electrical Power") + ": "
                    } Label {
                        text: powerString
                    } Label {
                        text: qsTr ("Nominal Voltage") + ": "
                    } Label {
                        text: maxVoltage + " V"
                    } Label {
                        text: qsTr ("Nominal Power Supply") + ": "
                    } Label {
                        text: maxPower + " W"
                    }
                }

                Canvas {
                    id: canvas
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    onPaint: {
                        var context = getContext('2d')
                        context.globalCompositeOperation = "source-over";

                        context.fillStyle = Globals.Colors.WindowBackground
                        context.fillRect (0, 0, canvas.width, canvas.height);
                        context.strokeStyle = Globals.Colors.HighlightColor

                        /* Calculate current voltage ratio with height */
                        var voltageLevel = voltage / maxVoltage
                        var initialHeight = height * 0.9

                        /* Calculate X and Y coordinates */
                        var xOffset = timer.pos
                        var yOffset = initialHeight - (voltageLevel * height)

                        /* Reset X if it is greater than the width */
                        if (xOffset > width) {
                            xOffset = 0
                            timer.pos = 0
                        }

                        /* Draw the point, but also extend it as a rectangle
                         * to "fill" the graph */
                        context.strokeRect (xOffset, yOffset, 0, yOffset)
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Globals.spacing

            Checkbox {
                checked: false
                id: showDashboardOnInit
                text: qsTr ("Show this window on application launch")
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr ("Close")
                onClicked: powerDashboard.close()
            }
        }
    }
}