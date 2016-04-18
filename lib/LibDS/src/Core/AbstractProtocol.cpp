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

#include "LibDS/DriverStation.h"
#include "LibDS/Core/AbstractProtocol.h"

using namespace DS_Core;

//==================================================================================================
// NetConsole warnings/information texts
//==================================================================================================

const QString INITIALIZED           = "<p>"
                                      "<font color=#888>** <font color=#AAA>%1</font> "
                                      "Initialized</font>"
                                      "</p>";
const QString PATIENCE_WITHOUT_TIME = "<p>"
                                      "<font color=#888>"
                                      "** It may take some time to detect the robot</font>"
                                      "</p>";
const QString PATIENCE_WITH_TIME    = "<p>"
                                      "<font color=#888>"
                                      "** It may take up to %1 seconds to detect the robot</font>"
                                      "</p>";
const QString IP_INFORMATION        = "<p>"
                                      "<font color=#888>"
                                      "** %1 robot IPs generated from %2 interfaces</font>"
                                      "</p>";

//==================================================================================================
// AbstractProtocol::AbstractProtocol
//==================================================================================================

AbstractProtocol::AbstractProtocol() {
    m_team                = 0;
    m_interfaces          = 0;
    m_radioIterator       = 0;
    m_sentFMSPackets      = 0;
    m_sentRobotPackets    = 0;

    m_alliance            = DS::kAllianceRed1;
    m_controlMode         = DS::kControlInvalid;
    m_communicationStatus = DS::kFailing;

    m_enabled             = false;
    m_operating           = false;
    m_robotCode           = false;
    m_fmsAttached         = false;
    m_emergencyStop       = false;
    m_radioConnected      = false;

    m_radioPing.setObjectName ("Radio Ping");
    m_robotPing.setObjectName ("Robot Ping");

    connect (&m_watchdog,  &Watchdog::timeout,
             this,         &AbstractProtocol::reset);
    connect (&m_robotPing, &QTcpSocket::stateChanged,
             this,         &AbstractProtocol::onPingResponse);
    connect (&m_radioPing, &QTcpSocket::stateChanged,
             this,         &AbstractProtocol::onPingResponse);
    connect (&m_sockets,   &SocketManager::fmsPacketReceived,
             this,         &AbstractProtocol::readFmsPacket);
    connect (&m_sockets,   &SocketManager::robotPacketReceived,
             this,         &AbstractProtocol::readRobotPacket);

    generateIpLists();
    m_watchdog.setTimeout (1000);
    QTimer::singleShot (200, Qt::CoarseTimer, this, SLOT (reset()));
    QTimer::singleShot (800, Qt::CoarseTimer, this, SLOT (initialize()));
}

//==================================================================================================
// AbstractProtocol::team
//==================================================================================================

int AbstractProtocol::team() const {
    return m_team;
}

//==================================================================================================
// AbstractProtocol::hasCode
//==================================================================================================

bool AbstractProtocol::hasCode() const {
    return m_robotCode;
}

//==================================================================================================
// AbstractProtocol::isOperating
//==================================================================================================

bool AbstractProtocol::isOperating() const {
    return m_operating;
}

//==================================================================================================
// AbstractProtocol::expirationTime
//==================================================================================================

int AbstractProtocol::expirationTime() const {
    return m_watchdog.expirationTime();
}

//==================================================================================================
// AbstractProtocol::sentFMSPackets
//==================================================================================================

int AbstractProtocol::sentFmsPackets() const {
    return m_sentFMSPackets;
}

//==================================================================================================
// AbstractProtocol::sentRobotPackets
//==================================================================================================

int AbstractProtocol::sentRobotPackets() const {
    return m_sentRobotPackets;
}

//==================================================================================================
// AbstractProtocol::isEnabled
//==================================================================================================

bool AbstractProtocol::isEnabled() const {
    return m_enabled;
}

//==================================================================================================
// AbstractProtocol::isConnectedToRobot
//==================================================================================================

bool AbstractProtocol::isConnectedToRobot() const {
    return communicationStatus() == DS::kFull;
}

//==================================================================================================
// AbstractProtocol::isConnectedToRadio
//==================================================================================================

bool AbstractProtocol::isConnectedToRadio() const {
    return m_radioConnected;
}

//==================================================================================================
// AbstractProtocol::sendDateTime
//==================================================================================================

bool AbstractProtocol::sendDateTime() const {
    return m_sendDateTime;
}

//==================================================================================================
// AbstractProtocol::hasVoltageBrownout
//==================================================================================================

bool AbstractProtocol::hasVoltageBrownout() const {
    return m_voltageBrownout;
}

//==================================================================================================
// AbstractProtocol::isEmergencyStopped
//==================================================================================================

bool AbstractProtocol::isEmergencyStopped() const {
    return m_emergencyStop;
}

//==================================================================================================
// AbstractProtocol::isFmsAttached
//==================================================================================================

bool AbstractProtocol::isFmsAttached() const {
    return m_fmsAttached;
}

//==================================================================================================
// AbstractProtocol::batteryVoltage
//==================================================================================================

float AbstractProtocol::batteryVoltage() const {
    return m_voltage;
}

//==================================================================================================
// AbstractProtocol::alliance
//==================================================================================================

DS::Alliance AbstractProtocol::alliance() const {
    return m_alliance;
}

//==================================================================================================
// AbstractProtocol::controlMode
//==================================================================================================

DS::ControlMode AbstractProtocol::controlMode() const {
    return m_controlMode;
}

//==================================================================================================
// AbstractProtocol::communicationStatus
//==================================================================================================

DS::DS_CommStatus AbstractProtocol::communicationStatus() const {
    return m_communicationStatus;
}

//==================================================================================================
// AbstractProtocol::joysticks
//==================================================================================================

QList<DS::Joystick>* AbstractProtocol::joysticks() const {
    return DriverStation::getInstance()->joysticks();
}

//==================================================================================================
// AbstractProtocol::radioAddress
//==================================================================================================

QString AbstractProtocol::radioAddress() {
    if (m_radioIPs.count() > m_radioIterator)
        return m_radioIPs.at (m_radioIterator);

    else
        return DS::getStaticIP (10, team(), 1);
}

//==================================================================================================
// AbstractProtocol::robotAddress
//==================================================================================================

QString AbstractProtocol::robotAddress() {
    return m_sockets.robotAddress();
}

//==================================================================================================
// AbstractProtocol::radioIPs
//==================================================================================================

QStringList AbstractProtocol::radioIPs() {
    return m_radioIPs;
}

//==================================================================================================
// AbstractProtocol::robotIPs
//==================================================================================================

QStringList AbstractProtocol::robotIPs() {
    return m_robotIPs;
}

//==================================================================================================
// AbstractProtocol::stop
//==================================================================================================

void AbstractProtocol::stop() {
    m_operating = false;
}

//==================================================================================================
// AbstractProtocol::start
//==================================================================================================

void AbstractProtocol::start() {
    m_operating = true;
}

//==================================================================================================
// AbstractProtocol::reset
//==================================================================================================

void AbstractProtocol::reset() {
    /* Only called if the protocol is allowed to operate */
    if (isOperating()) {
        /* Custom reset procedures for each protocol */
        resetProtocol();

        /* Emit appropiate signals */
        updateVoltage (0, 0);
        updateRobotCode (false);
        updateRadioStatus (false);
        updateFmsAttached (false);
        updateSendDateTime (false);
        updateCommStatus (DS::kFailing);

        /* Update the radio IP */
        if (!isConnectedToRadio()) {
            if (m_radioIterator < m_radioIPs.count())
                ++m_radioIterator;
            else
                m_radioIterator = 0;
        }

        /* Ping robot & radio */
        pingRadio();
        pingRobot();

        /* Scan the next round of IP addresses */
        if (robotAddress().isEmpty())
            m_sockets.refreshIPs();
    }
}

//==================================================================================================
// AbstractProtocol::setTeam
//==================================================================================================

void AbstractProtocol::setTeam (int team) {
    if (team != m_team) {
        m_team = team;
        generateIpLists();
        DS::log (DS::kLibLevel, "Team number set to: " + QString::number (m_team));
    }
}

//==================================================================================================
// AbstractProtocol::setEnabled
//==================================================================================================

void AbstractProtocol::setEnabled (bool enabled) {
    m_enabled = enabled;
    emit enabledChanged (isEnabled());
}

//==================================================================================================
// AbstractProtocol::setEmergencyStop
//==================================================================================================

void AbstractProtocol::setEmergencyStop (bool emergency_stop) {
    m_emergencyStop = emergency_stop;
    emit emergencyStoppedChanged (isEmergencyStopped());

    if (isEmergencyStopped())
        QTimer::singleShot (500, this, SLOT (disableEmergencyStop()));

    emit emergencyStopped();
}

//==================================================================================================
// AbstractProtocol::setRobotAddress
//==================================================================================================

void AbstractProtocol::setRobotAddress (QString address) {
    m_sockets.setRobotAddress (address);
}

//==================================================================================================
// AbstractProtocol::setAlliance
//==================================================================================================

void AbstractProtocol::setAlliance (DS::Alliance alliance) {
    m_alliance = alliance;
}

//==================================================================================================
// AbstractProtocol::setControlMode
//==================================================================================================

void AbstractProtocol::setControlMode (DS::ControlMode mode) {
    if (!isEmergencyStopped()) {
        m_controlMode = mode;
        emit controlModeChanged (controlMode());
    }
}

//==================================================================================================
// AbstractProtocol::updateRobotCode
//==================================================================================================

void AbstractProtocol::updateRobotCode (bool available) {
    /* Robot code just crashed/failed */
    if (m_robotCode && !available)
        setEnabled (false);

    /* Update DS information */
    m_robotCode = available;
    emit codeChanged (m_robotCode);
}

//==================================================================================================
// AbstractProtocol::updateSendDateTime
//==================================================================================================

void AbstractProtocol::updateSendDateTime (bool sendDT) {
    m_sendDateTime = sendDT;
}

//==================================================================================================
// AbstractProtocol::updateFmsAttached
//==================================================================================================

void AbstractProtocol::updateFmsAttached (bool attached) {
    m_fmsAttached = attached;
    emit fmsChanged (m_fmsAttached);
}


//==================================================================================================
// AbstractProtocol::updateRadioStatus
//==================================================================================================

void AbstractProtocol::updateRadioStatus (bool connected) {
    m_radioConnected = connected;
    emit radioCommChanged (m_radioConnected);
}

//==================================================================================================
// AbstractProtocol::updateVoltageBrownout
//==================================================================================================

void AbstractProtocol::updateVoltageBrownout (bool brownout) {
    m_voltageBrownout = brownout;
    emit voltageBrownoutChanged (m_voltageBrownout);
}

//==================================================================================================
// AbstractProtocol::updateVoltage
//==================================================================================================

void AbstractProtocol::updateVoltage (QString integer, QString decimal) {
    /* Voltage is smaller than 10v, add 0 before the digit (e.g. to 09) */
    if (integer.length() < 2)
        integer.prepend ("0");

    /* Decimal voltage is less than 0.1v, add 0 to decimal (e.g 0.01) */
    if (decimal.length() < 2)
        decimal.prepend ("0");

    /* Decimal is too detailed, obtain only first two digits */
    else if (decimal.length() > 2)
        decimal = QString (decimal.at (0)) + QString (decimal.at (1));

    /* Construct voltage string */
    QString voltage = QString ("%1.%2").arg (integer, decimal);

    /* Update values & emit signals */
    m_voltage = voltage.toFloat();
    emit voltageChanged (voltage);
}

//==================================================================================================
// AbstractProtocol::updateCommStatus
//==================================================================================================

void AbstractProtocol::updateCommStatus (DS::DS_CommStatus status) {
    m_communicationStatus = status;
    emit communicationsChanged (m_communicationStatus);
}

//==================================================================================================
// AbstractProtocol::pingRobot
//==================================================================================================

void AbstractProtocol::pingRobot() {
    m_robotPing.abort();
    m_robotPing.connectToHost (robotAddress(), tcpProbesPort(), QTcpSocket::ReadOnly);
}

//==================================================================================================
// AbstractProtocol::pingRadio
//==================================================================================================

void AbstractProtocol::pingRadio() {
    m_radioPing.abort();
    m_radioPing.connectToHost (radioAddress(), 80, QTcpSocket::ReadOnly);
}

//==================================================================================================
// AbstractProtocol::initialize
//==================================================================================================

void AbstractProtocol::initialize() {
    /* Get total scanning time in seconds*/
    int ips = robotIPs().count() > 0 ? robotIPs().count() : 1;
    int time = ((ips * expirationTime()) / m_sockets.scannerCount()) / 1000;

    /* Decide which patience message to use */
    QString be_patient = PATIENCE_WITHOUT_TIME;
    if (time > 5 && time < 60)
        be_patient = PATIENCE_WITH_TIME.arg (time);

    /* Display the message */
    DS::sendMessage (INITIALIZED.arg (name()));
    DS::sendMessage (be_patient);
    DS::sendMessage (IP_INFORMATION.arg (m_robotIPs.count()).arg (m_interfaces));

    /* Begin the packet creation loop */
    sendFmsPacket();
    sendRobotPacket();
}

//==================================================================================================
// AbstractProtocol::sendFmsPacket
//==================================================================================================

void AbstractProtocol::sendFmsPacket() {
    if (isOperating()) {
        m_sentFMSPackets += 1;
        m_sockets.sendFmsPacket (generateFmsPacket());
    }

    QTimer::singleShot (1000 / fmsFrequency(), Qt::PreciseTimer, this, SLOT (sendFmsPacket()));
}

//==================================================================================================
// AbstractProtocol::sendRobotPacket
//==================================================================================================

void AbstractProtocol::sendRobotPacket() {
    if (isOperating()) {
        m_sentRobotPackets += 1;
        m_sockets.sendRobotPacket (generateRobotPacket());
    }

    QTimer::singleShot (1000 / robotFrequency(), Qt::PreciseTimer, this, SLOT (sendRobotPacket()));
}


//==================================================================================================
// AbstractProtocol::generateIpLists
//==================================================================================================

void AbstractProtocol::generateIpLists() {
    /* Clear the IPs lists */
    m_interfaces = 0;
    m_robotIPs.clear();
    m_radioIPs.clear();

    /* Generate radio IPs */
    m_radioIPs.append (additionalRadioIPs());
    m_radioIPs.append (DS::getStaticIP (10, team(), 1));

    /* Generate robot IPs */
    m_robotIPs.append (additionalRobotIPs());
    m_robotIPs.append ("127.0.0.1");

    /* Generate robot IPs based on the current IP(s) of the computer */
    foreach (const QNetworkInterface& interface, QNetworkInterface::allInterfaces()) {
        bool isUp       = (bool) (interface.flags() & QNetworkInterface::IsUp);
        bool isRunning  = (bool) (interface.flags() & QNetworkInterface::IsRunning);

        /* Only take into account useful network interfaces */
        if (isUp && isRunning) {
            ++m_interfaces;
            foreach (const QNetworkAddressEntry& address, interface.addressEntries()) {
                QStringList numbers = address.ip().toString().split (".");
                bool valid = (address.ip() != QHostAddress ("127.0.0.1")) &&
                             (address.ip().protocol() == QAbstractSocket::IPv4Protocol);

                /* Ensure that the current IP is IPv4 and is valid */
                if (numbers.count() == 4 && valid) {
                    QString base = QString ("%1.%2.%3.")
                                   .arg (numbers.at (0))
                                   .arg (numbers.at (1))
                                   .arg (numbers.at (2));

                    /* Add all other IPs in the selected network */
                    for (int i = 1; i < 255; ++i)
                        m_robotIPs.append (base + QString::number (i));
                }
            }
        }
    }

    /* Remove duplicates and empty strings from the list */
    QStringList fixed_list;
    foreach (const QString& string, m_robotIPs)
        if (!string.isEmpty() && !fixed_list.contains (string))
            fixed_list.append (string);

    /* Re-assing the 'fixed' IP list to the robot IP list */
    m_robotIPs = fixed_list;

    /* Re-configure the network scanner  ports */
    m_sockets.setFmsInputPort (fmsInputPort());
    m_sockets.setFmsOutputPort (fmsOutputPort());
    m_sockets.setRobotInputPort (robotInputPort());
    m_sockets.setRobotOutputPort (robotOutputPort());

    /* Update the IP lists */
    m_sockets.setRobotIPs (m_robotIPs);

    /* Log information */
    DS::log (DS::kLibLevel, QString ("Generated %1 radio IPs").arg (m_radioIPs.count()));
    DS::log (DS::kLibLevel, QString ("Generated %1 robot IPs").arg (m_robotIPs.count()));
}

//==================================================================================================
// AbstractProtocol::disableEmergencyStop
//==================================================================================================

void AbstractProtocol::disableEmergencyStop() {
    setEmergencyStop (false);
}

//==================================================================================================
// AbstractProtocol::readFmsSPacket
//==================================================================================================

void AbstractProtocol::readFmsPacket (QByteArray data) {
    if (data.isEmpty())
        return;

    if (interpretFmsPacket (data) && !isFmsAttached())
        updateFmsAttached (true);
}

//==================================================================================================
// AbstractProtocol::readRobotPacket
//==================================================================================================

void AbstractProtocol::readRobotPacket (QByteArray data) {
    if (data.isEmpty())
        return;

    if (!isConnectedToRobot()) {
        if (controlMode() != DS::kControlInvalid)
            setControlMode (controlMode());
        else
            setControlMode (DS::kControlTeleoperated);

        setEnabled (isEnabled());
        updateCommStatus (DS::kFull);
        DS::log (DS::kLibLevel, "Robot/DS connection established!");

        getRobotInformation();
    }

    if (interpretRobotPacket (data))
        m_watchdog.restart();
}

//==================================================================================================
// AbstractProtocol::onPingResponse
//==================================================================================================

void AbstractProtocol::onPingResponse (QAbstractSocket::SocketState state) {
    bool isAlive = (state == QAbstractSocket::ConnectedState);

    /* Emitter is robot socket */
    if (sender()->objectName() == m_robotPing.objectName()) {
        if (communicationStatus() == DS::kFailing && isAlive)
            updateCommStatus (DS::kPartial);
    }

    /* Emitter is radio socket */
    else if (sender()->objectName() == m_radioPing.objectName())
        updateRadioStatus (isAlive);
}