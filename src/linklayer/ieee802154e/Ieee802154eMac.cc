/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright   (C) 2019  Institute of Communication Networks (ComNets),
 *                       Hamburg University of Technology (TUHH)
 *                       Leo Krueger, Louis Yin
 *
 * This work is based on Ieee802154Mac.cc from INET 4.1:
 *
 * Author:     Jerome Rousselot, Marcel Steine, Amre El-Hoiydi,
 *             Marc Loebbers, Yosia Hadisusanto, Andreas Koepke
 *
 * Copyright:  (C) 2009 T.U. Eindhoven
 *             (C) 2007-2009 CSEM SA
 *             (C) 2004,2005,2006
 *             Telecommunication Networks Group (TKN) at Technische
 *             Universitaet Berlin, Germany.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cassert>

#include "Ieee802154eMac.h"
#include "inet/common/FindModule.h"
#include "inet/common/INETMath.h"
#include "inet/common/INETUtils.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/ProtocolGroup.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "Ieee802154eMacHeader_m.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/common/Units.h"
#include "inet/common/packet/Message.h"
#include "inet/physicallayer/common/packetlevel/RadioMedium.h"
#include "./sixtisch/SixpHeaderChunk_m.h"
#include "../../common/VirtualLinkTag_m.h"

namespace tsch {

using namespace inet::physicallayer;
using namespace omnetpp;
using namespace inet;

Define_Module(Ieee802154eMac);

void Ieee802154eMac::initialize(int stage) {
    inet::MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        sixTopSublayerInGateId = findGate("sixTopSublayerInGate");
        sixTopSublayerOutGateId = findGate("sixTopSublayerOutGate");
        //sixTopSublayerControlInGateId = findGate("sixTopSublayerControlInGate");
        sixTopSublayerControlOutGateId = findGate("sixTopSublayerControlOutGate");
        useMACAcks = par("useMACAcks");
        sifs = par("sifs");
        macTsTimeslotLength = par("macTsTimeslotLength");
        headerLength = par("headerLength");
        transmissionAttemptInterruptedByRx = false;
        nbTxFrames = 0;
        nbRxFrames = 0;
        nbMissedAcks = 0;
        nbTxAcks = 0;
        nbRecvdAcks = 0;
        nbDroppedFrames = 0;
        nbDuplicates = 0;
        nbBackoffs = 0;
        backoffValues = 0;
        macMaxFrameRetries = par("macMaxFrameRetries");
        macAckWaitDuration = par("macAckWaitDuration");
        ccaDetectionTime = par("ccaDetectionTime");
        useCCA = par("useCCA");
        rxSetupTime = par("rxSetupTime");
        aTurnaroundTime = par("aTurnaroundTime");
        channelSwitchingTime = par("channelSwitchingTime");
        bitrate = par("bitrate");
        ackLength = par("ackLength");
        ackMessage = nullptr;
        currentAsn = 0;
        currentLink = nullptr;
        currentChannel = 0;


        //init parameters for backoff method
        std::string backoffMethodStr = par("backoffMethod").stdstringValue();
        if (backoffMethodStr == "exponential") {
            backoffMethod = EXPONENTIAL;
        } else {
            throw cRuntimeError(
                    "Unknown backoff method \"%s\".\
                   Only \"exponential\", implemented \
                   so far.",
                    backoffMethodStr.c_str());
        }
        NB = 0;

        // initialize the timers
        slotTimer = new cMessage("timer-slot");
        ccaTimer = new cMessage("timer-cca");
        sifsTimer = new cMessage("timer-sifs");
        rxAckTimer = new cMessage("timer-rxAck");
        hoppingTimer = new cMessage("timer-hopping");
        slotendTimer = new cMessage("timer-slotend");
        macState = IDLE_1;
        txAttempts = 0;

        statisticTemplate = getProperties()->get("statisticTemplate", "nbStats");

    } else if (stage == INITSTAGE_LINK_LAYER) {
        EV_DETAIL << "We are in INISTAGELINK LAYER" << endl;
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"),
                this);
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radio = check_and_cast<IRadio *>(radioModule);

        //check parameters for consistency
        //aTurnaroundTime should match (be equal or bigger) the RX to TX
        //switching time of the radio
        if (radioModule->hasPar("timeRXToTX")) {
            simtime_t rxToTx = radioModule->par("timeRXToTX");
            if (rxToTx > aTurnaroundTime) {
                throw cRuntimeError(
                        "Parameter \"aTurnaroundTime\" (%f) does not match"
                                " the radios RX to TX switching time (%f)! It"
                                " should be equal or bigger",
                        SIMTIME_DBL(aTurnaroundTime), SIMTIME_DBL(rxToTx));
            }
        }
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

        hopping = dynamic_cast<TschHopping*>(getModuleByPath(
                "^.channelHopping"));

        // get our slotframe
        schedule = dynamic_cast<TschSlotframe*>(getModuleByPath("^.schedule"));

        neighbor = dynamic_cast<TschNeighbor*>(getModuleByPath("^.neighbor"));

        // minimal schedule as per 6TiSCH some document TODO

        schedule->xmlSchedule();
        schedule->printSlotframe();

        asn.setMacTsTimeslotLength(macTsTimeslotLength);
        asn.setReference(simTime() + 0.1);
        scheduleAt(simTime() + 0.1, slotTimer);

        EV_DETAIL << "QueueLength = " << neighbor->getQueueLength() << " bitrate = " << bitrate << endl;
        EV_DETAIL << "Finished tsch init stage 1." << endl;
    }
}

void Ieee802154eMac::finish() {
    recordScalar("nbTxFrames", nbTxFrames);
    recordScalar("nbRxFrames", nbRxFrames);
    recordScalar("nbDroppedFrames", nbDroppedFrames);
    recordScalar("nbMissedAcks", nbMissedAcks);
    recordScalar("nbRecvdAcks", nbRecvdAcks);
    recordScalar("nbTxAcks", nbTxAcks);
    recordScalar("nbDuplicates", nbDuplicates);
    if (nbBackoffs > 0) {
        recordScalar("meanBackoff", backoffValues / nbBackoffs);
    } else {
        recordScalar("meanBackoff", 0);
    }
    recordScalar("nbBackoffs", nbBackoffs);
    recordScalar("backoffDurations", backoffValues);
}

Ieee802154eMac::~Ieee802154eMac() {
    cancelAndDelete(slotTimer);
    cancelAndDelete(ccaTimer);
    cancelAndDelete(sifsTimer);
    cancelAndDelete(rxAckTimer);
    cancelAndDelete(slotendTimer);
    cancelAndDelete(hoppingTimer);
    if (ackMessage){
        delete ackMessage;
    }
    neighbor->clearQueue();
}

void Ieee802154eMac::emitSignal(signal_names signalName)
{
    std::string name;

    switch (signalName) {
        case NBTXFRAMES:
            name += "nbTxFrames";
            break;
        case NBMISSEDACKS:
            name += "nbMissedAcks";
            break;
        case NBRECVDACKS:
            name += "nbRecvdAcks";
            break;
        case NBRXFRAMES:
            name += "nbRxFrames";
            break;
        case NBTXACKS:
            name += "nbTxAcks";
            break;
        case NBDUPLICATES:
            name += "nbDuplicates";
            break;
        case NBSLOT:
            name += "nbSlot";
            break;
        default:
            EV << "EmitSignal Error ! Unknown signal id:" << signalName << endl;
            return;
    }

    std::vector<std::string> scopes;
    scopes.push_back(std::string("chan-"));
    scopes.push_back(std::string("link-"));
    scopes.push_back(std::string("neigh-"));

    for (auto & scope: scopes) {
        std::string signalName(scope);

        if (scope == "chan-") signalName += std::to_string(currentChannel);
        if (scope == "link-") signalName += currentLink->slug();
        if (scope == "neigh-") signalName += currentLink->getAddr().str();

        signalName = name + "-" + signalName;

        if (std::find(registeredSignals.begin(), registeredSignals.end(), signalName) == registeredSignals.end()) {
            getEnvir()->addResultRecorders(this, registerSignal(signalName.c_str()), signalName.c_str(), statisticTemplate);
            registeredSignals.push_back(signalName);
            par("numSignals").setIntValue(par("numSignals").intValue() + 1); // TODO somewhat hackish, let's see
        }

        emit(registerSignal(signalName.c_str()), (int) currentAsn);
    }

}


void Ieee802154eMac::configureInterfaceEntry() {
    MacAddress address = parseMacAddressParameter(par("address"));

    // data rate
    interfaceEntry->setDatarate(bitrate);

    // generate a link-layer address to be used as interface token for IPv6
    interfaceEntry->setMacAddress(address);
    interfaceEntry->setInterfaceToken(address.formInterfaceIdentifier());

    // capabilities
    interfaceEntry->setMtu(par("mtu"));
    interfaceEntry->setMulticast(true);
    interfaceEntry->setBroadcast(true);
}

bool Ieee802154eMac::isUpperMessage(cMessage *message)
{
    return (message->getArrivalGateId() == upperLayerInGateId) || (message->getArrivalGateId() == sixTopSublayerInGateId);
}

/**
 * Encapsulates the message to be transmitted and pass it on
 * to the FSM main method for further processing.
 */
void Ieee802154eMac::handleUpperPacket(Packet *packet) {
    //MacPkt*macPkt = encapsMsg(msg);
    auto macPkt = makeShared<Ieee802154eMacHeader>();
    auto proto = packet->getTag<PacketProtocolTag>()->getProtocol();
    auto virtualLinkTag = packet->findTag<VirtualLinkTagReq>();
    int linkId;
    if (virtualLinkTag != nullptr){
        linkId = virtualLinkTag->getVirtualLinkID();
    }else{
        linkId = 0;
    }
    macPkt->setVirtualLinkID(linkId);
    assert(headerLength % 8 == 0);
    macPkt->setChunkLength(b(headerLength));
    MacAddress dest = packet->getTag<MacAddressReq>()->getDestAddress();
    EV_DETAIL << "TSCH received a message from upper layer, name is "
                     << packet->getName() << ", CInfo removed, mac addr="
                     << dest << endl;
    macPkt->setNetworkProtocol(ProtocolGroup::ethertype.getProtocolNumber(proto));
    macPkt->setDestAddr(dest);
    delete packet->removeControlInfo();
    macPkt->setSrcAddr(interfaceEntry->getMacAddress());
    if (useMACAcks) {

        if (SeqNrParent.count(dest) == 0) {
            //no record of current parent -> add next sequence number to map
            SeqNrParent[dest][linkId] = 1;
            macPkt->setSequenceId(0);
            EV_DETAIL << "Adding a new parent to the map of Sequence numbers:"
                             << dest << " with linkId " << linkId << endl;
        } else {
            if (SeqNrParent[dest].count(linkId) == 0) {
                //no record of current linkId -> add next sequence number to map
                SeqNrParent[dest][linkId] = 1;
                macPkt->setSequenceId(0);
                EV_DETAIL << "Adding a new link to the map of Sequence numbers:"
                               << linkId << endl;
            } else {
                macPkt->setSequenceId(SeqNrParent[dest][linkId]);
                EV_DETAIL << "Packet sent with sequence number = "
                                 << SeqNrParent[dest][linkId] << endl;
                SeqNrParent[dest][linkId]++;
            }
        }
    }

    //RadioAccNoise3PhyControlInfo *pco = new RadioAccNoise3PhyControlInfo(bitrate);
    //macPkt->setControlInfo(pco);
    packet->insertAtFront(macPkt);
    // TODO we are using protocol ieee802154 for now
    packet->getTag<PacketProtocolTag>()->setProtocol(&Protocol::ieee802154);
    EV_DETAIL << "Pkt encapsulated, length: " << macPkt->getChunkLength()
                     << "\n";
    if(neighbor->add2Queue(packet, dest, linkId)){
        EV_DETAIL << "Added packet to queue" << endl;
    }else{
        EV_DETAIL << "Packet is dropped due to Queue Overflow" << endl;
        PacketDropDetails details;
        details.setReason(QUEUE_OVERFLOW);
        details.setLimit(neighbor->getQueueLength());
        emit(packetDroppedSignal, packet, &details);
        delete packet;
    }
}

void Ieee802154eMac::updateStatusIdle(t_mac_event event, cMessage *msg) {
    switch (event) {
    case EV_TIMER_SLOT: {
        EV_DETAIL
                         << "(1) FSM State IDLE_1, EV_TIMER_SLOT: startTimerSlot -> idle."
                         << endl;
        // directly schedule next slot
        startTimer(TIMER_SLOT);

        // start of new slot
        // Done in  startTimer(Timer_Slot);
        //currentAsn = asn.getAsn(simTime());
        currentLink = schedule->getLinkFromASN(currentAsn);
        if (currentLink == nullptr) {
            return;
        }
        currentChannel = hopping->channel(currentAsn, currentLink->getChannelOffset());
        auto freq = hopping->channelToCenterFrequency(currentChannel);
        int currentVirtualLinkID;
        if(dynamic_cast<TschVirtualLink*>(currentLink) != nullptr){
            currentVirtualLinkID = (dynamic_cast<TschVirtualLink*>(currentLink))->getVirtualLink();
        }else{
            currentVirtualLinkID = 0;
        }

        EV_DETAIL << currentLink->str() << endl;

        // unconditionally emit a signal at slot start when we already have link infos
        emitSignal(NBSLOT);

        neighbor->reset();
        auto queueSize = neighbor->checkVirtualQueueSizeAt(currentLink->getAddr(), currentVirtualLinkID);
        EV_DETAIL << "We are in ASN " << currentAsn << " queue size "
                         << queueSize
                         << " channel " << currentChannel << " frequency " << freq << endl;
        EV_DETAIL << "This Node is: " << interfaceEntry->getMacAddress() << endl;
        neighbor->printQueue();

        if (currentLink->isTx()) {
            if (queueSize > 0) {
                neighbor->setSelectedQueue(currentLink->getAddr(),currentVirtualLinkID);
                // Dedicated link
                if (!currentLink->isShared()) {
                    neighbor->setDedicated(true);
                    updateMacState(CCA_3);
                    startTimer(TIMER_CCA);
                    configureRadio(freq, IRadio::RADIO_MODE_RECEIVER);
                }
                // Shared link
                else if (currentLink->isShared()){
                    // Backoff == 0
                    if(neighbor->getCurrentTschCSMA()->checkBackoff()){
                        updateMacState(CCA_3);
                        startTimer(TIMER_CCA);
                        configureRadio(freq, IRadio::RADIO_MODE_RECEIVER);
                    }
                }
                // Nothing to send -> RX
                else if (currentLink->isRx()) {
                    updateMacState(RECEIVEFRAME_6);
                    startTimer(TIMER_SLOTEND);
                    configureRadio(freq, IRadio::RADIO_MODE_RECEIVER);
                }
            }
            // Its a transmission link (and shared and with broadcast address) but the specific queue is empty,
            // else it would be impossible to reach this point
            else if (currentLink->isShared() && currentLink->getAddr().isBroadcast()) {
                // Check if a non shared link can use the unused shared slotframe to transmit packets
                std::vector<inet::MacAddress> tempMacDedicated;
                // Fill the temporary vector with the MacAddress of dedicated links from the schedule
                tempMacDedicated = schedule->getMacDedicated();
                //Check if the specific queue are filled and select one
                if (neighbor->checkAndselectQueue(tempMacDedicated, schedule)) {
                    updateMacState(CCA_3);
                    startTimer(TIMER_CCA);
                    configureRadio(freq, IRadio::RADIO_MODE_RECEIVER);
                }
                // Nothing to send -> RX
                else if (currentLink->isRx()) {
                    updateMacState(RECEIVEFRAME_6);
                    startTimer(TIMER_SLOTEND);
                    configureRadio(freq, IRadio::RADIO_MODE_RECEIVER);
                }
            }
        }
        // Nothing to send -> RX
        else if (currentLink->isRx()) {
            updateMacState(RECEIVEFRAME_6);
            startTimer(TIMER_SLOTEND);
            configureRadio(freq, IRadio::RADIO_MODE_RECEIVER);
        }
        // Decrement backoff window for all queues directed at link->getAddr
        if(currentLink->isTx() && currentLink->isShared()){
            neighbor->updateAllBackoffWindows(currentLink->getAddr(),schedule);
        }
        break;
    }
    default:
        fsmError(event, msg);
        break;
    }
}

void Ieee802154eMac::configureRadio(Hz carrierFrequency /*= NAN*/,
        int mode /*= -1*/) {
    auto configureCommand = new ConfigureRadioCommand();
    auto request = new Message("changeChannel", RADIO_C_CONFIGURE);

    configureCommand->setCarrierFrequency(carrierFrequency);
    configureCommand->setRadioMode(mode);
    request->setControlInfo(configureCommand);

    sendDown(request);
}

void Ieee802154eMac::flushQueue() {
}


void Ieee802154eMac::attachSignal(Packet *mac, simtime_t_cref startTime) {
    simtime_t duration = mac->getBitLength() / bitrate;
    mac->setDuration(duration);
}

void Ieee802154eMac::updateStatusCCA(t_mac_event event, cMessage *msg) {
    switch (event) {
    case EV_TIMER_CCA: {
        EV_DETAIL << "(25) FSM State CCA_3, EV_TIMER_CCA" << endl;
        bool isIdle = radio->getReceptionState()
                == IRadio::RECEPTION_STATE_IDLE;
        if (isIdle) {
            EV_DETAIL
                             << "(3) FSM State CCA_3, EV_TIMER_CCA, [Channel Idle]: -> TRANSMITFRAME_4."
                             << endl;
            updateMacState(TRANSMITFRAME_4);
            radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
            Packet *mac =  check_and_cast<Packet *>(neighbor->getCurrentNeighborQueueFirstPacket()->dup());
            attachSignal(mac, simTime() + aTurnaroundTime);
            // give time for the radio to be in Tx state before transmitting
            sendDelayed(mac, aTurnaroundTime, lowerLayerOutGateId);
            nbTxFrames++;

            emitSignal(NBTXFRAMES);

            EV << "Number of transmitted frames: " << nbTxFrames << endl;
        } else {
            // Channel was busy, increment 802.15.4 backoff timers as specified.
            EV_DETAIL << "(7) FSM State CCA_3, EV_TIMER_CCA, [Channel Busy]: "
                             << " skipping slot." << endl;

            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            manageFailedTX();
            updateMacState(IDLE_1);
        }
        break;
    }
    default:
        fsmError(event, msg);
        break;
    }
}

void Ieee802154eMac::updateStatusTransmitFrame(t_mac_event event,
        cMessage *msg) {
    if (event == EV_FRAME_TRANSMITTED) {
        Packet *packet =
                neighbor->getCurrentNeighborQueueFirstPacket();
        const auto& csmaHeader = packet->peekAtFront<Ieee802154eMacHeader>();

        bool expectAck = useMACAcks;
        if (!csmaHeader->getDestAddr().isBroadcast()
                && !csmaHeader->getDestAddr().isMulticast()) {
            //unicast
            EV_DETAIL << "(4) FSM State TRANSMITFRAME_4, "
                             << "EV_FRAME_TRANSMITTED [Unicast]: ";
        } else {
            //broadcast
            EV_DETAIL << "(27) FSM State TRANSMITFRAME_4, EV_FRAME_TRANSMITTED "
                             << " [Broadcast]";
            expectAck = false;
        }

        if (expectAck) {
            EV_DETAIL << "RadioSetupRx -> WAITACK." << endl;
            radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            updateMacState(WAITACK_5);
            startTimer(TIMER_RX_ACK);
        } else {
            EV_DETAIL << ": RadioSetupSleep..." << endl;
            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            neighbor->removeFirstPacketFromQueue();
            delete packet;
            updateMacState(IDLE_1);
        }
        delete msg;
    } else {
        fsmError(event, msg);
    }
}

void Ieee802154eMac::updateStatusWaitAck(t_mac_event event, cMessage *msg) {
    assert(useMACAcks);
    switch (event) {
    case EV_ACK_RECEIVED: {
        EV_DETAIL << "(5) FSM State WAITACK_5, EV_ACK_RECEIVED: "
                         << " ProcessAck..." << endl;
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        if (rxAckTimer->isScheduled())
            cancelEvent(rxAckTimer);
        cMessage *mac =
                neighbor->getCurrentNeighborQueueFirstPacket();
        neighbor->removeFirstPacketFromQueue();
        if(neighbor->isDedicated()){
            if(neighbor->getCurrentNeighborQueueSize() == 0){
                neighbor->terminateCurrentTschCSMA();
            }
            neighbor->reset();
        }else{
            neighbor->terminateCurrentTschCSMA();
        }
        //PacketDropDetails details;
        //details.setReason(RETRY_LIMIT_REACHED);
        //details.setLimit(macMaxFrameRetries);
        emit(packetSentSignal, mac, nullptr); //&details);
        delete mac;
        delete msg;
        updateMacState(IDLE_1);
        break;
    }
    case EV_ACK_TIMEOUT:
        EV_DETAIL << "(12) FSM State WAITACK_5, EV_ACK_TIMEOUT:"
                         << " start TschCSMA,incrementCounter/dropPacket"
                         << endl;
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

        if(!neighbor->getCurrentTschCSMAStatus()){
            neighbor->startTschCSMA();
        }else{
            manageFailedTX();
        }
        updateMacState(IDLE_1);
        break;

    case EV_BROADCAST_RECEIVED:
    case EV_FRAME_RECEIVED:
    case EV_DUPLICATE_RECEIVED:
        EV_DETAIL << "Error ! Received a frame during SIFS !" << endl;
        decapsulate(check_and_cast<Packet *>(msg));
        sendUp(msg);
        break;

    default:
        fsmError(event, msg);
        break;
    }
}

void Ieee802154eMac::manageFailedTX() {
    neighbor->failedTX();
    if ((int) macMaxFrameRetries
            < (neighbor->getCurrentTschCSMA()->getNB())) {
        EV_DETAIL << "Packet was transmitted " << macMaxFrameRetries
                                 << " times and an ACK was never received. The packet is dropped."
                                 << endl;
                cMessage *mac =
                        neighbor->getCurrentNeighborQueueFirstPacket();
                neighbor->removeFirstPacketFromQueue();
                neighbor->terminateCurrentTschCSMA();
                PacketDropDetails details;
                details.setReason(RETRY_LIMIT_REACHED);
                details.setLimit(macMaxFrameRetries);
                emit(packetDroppedSignal, mac, &details);
                emit(linkBrokenSignal, mac);
                delete mac;
    }
    neighbor->reset();
}




void Ieee802154eMac::updateStatusReceiveFrame(t_mac_event event,
        omnetpp::cMessage *msg) {
    switch (event) {
    case EV_DUPLICATE_RECEIVED:
        EV_DETAIL << "(26) FSM State RECEIVEFRAME_6, EV_DUPLICATE_RECEIVED:";

        if (slotendTimer->isScheduled())
            cancelEvent(slotendTimer);

        if (useMACAcks) {
            EV_DETAIL << " setting up radio tx -> WAITSIFS." << endl;

            radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
            updateMacState(WAITSIFS_7);
            startTimer(TIMER_SIFS);
        } else {
            EV_DETAIL << " going to sleep" << endl;

            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            updateMacState(IDLE_1);
        }
        delete msg;
        break;

    case EV_FRAME_RECEIVED:
        EV_DETAIL << "(26) FSM State RECEIVEFRAME_6, EV_FRAME_RECEIVED:";

        if (slotendTimer->isScheduled())
            cancelEvent(slotendTimer);

        if (useMACAcks) {
            EV_DETAIL << " setting up radio tx -> WAITSIFS." << endl;
            // suspend current transmission attempt,
            // transmit ack,

            radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
            updateMacState(WAITSIFS_7);
            startTimer(TIMER_SIFS);
        } else {
            EV_DETAIL << " going to sleep." << endl;

            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            updateMacState(IDLE_1);
        }
        decapsulate(check_and_cast<Packet *>(msg));

        sendUp(msg);
        break;
    case EV_BROADCAST_RECEIVED:
        EV_DETAIL << "(24) FSM State RECEIVEFRAME_6, EV_BROADCAST_RECEIVED:"
                         << " going to sleep." << endl;

        if (slotendTimer->isScheduled())
            cancelEvent(slotendTimer);

        decapsulate(check_and_cast<Packet *>(msg));

        sendUp(msg);
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        updateMacState(IDLE_1);
        break;

    case EV_TIMER_SLOTEND:
        EV_DETAIL << "(24) FSM State RECEIVEFRAME_6, EV_TIMER_SLOTEND:"
                         << " Nothing received within slot. Going to sleep" << endl;
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        updateMacState(IDLE_1);
        break;

    default:
        fsmError(event, msg);
        break;
    }
}

void Ieee802154eMac::updateStatusSIFS(t_mac_event event, cMessage *msg) {
    assert(useMACAcks);

    switch (event) {
    case EV_TIMER_SIFS:
        EV_DETAIL << "(17) FSM State WAITSIFS_7, EV_TIMER_SIFS:"
                         << " sendAck -> TRANSMITACK." << endl;
        updateMacState(TRANSMITACK_8);
        attachSignal(ackMessage, simTime());
        sendDown(ackMessage);
        nbTxAcks++;

        emitSignal(NBTXACKS);

        EV << "Number of transmitted Acks: " << nbTxAcks << endl;
        //        sendDelayed(ackMessage, aTurnaroundTime, lowerLayerOut);
        ackMessage = nullptr;
        break;

    case EV_BROADCAST_RECEIVED:
    case EV_FRAME_RECEIVED:
        EV << "Error ! Received a frame during SIFS !" << endl;
        decapsulate(check_and_cast<Packet *>(msg));
        sendUp(msg);
        break;

    default:
        fsmError(event, msg);
        break;
    }
}

void Ieee802154eMac::updateStatusTransmitAck(t_mac_event event, cMessage *msg) {
    assert(useMACAcks);

    if (event == EV_FRAME_TRANSMITTED) {
        EV_DETAIL << "(19) FSM State TRANSMITACK_8, EV_FRAME_TRANSMITTED" << endl;
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        delete msg;
        updateMacState(IDLE_1);
    } else {
        fsmError(event, msg);
    }
}

/**
 * Updates state machine.
 */
void Ieee802154eMac::executeMac(t_mac_event event, cMessage *msg) {
    EV_DETAIL << "In executeMac" << endl;
    switch (macState) {
    case IDLE_1:
        updateStatusIdle(event, msg);
        break;

    case HOPPING_2:
        //updateStatusHopping(event, msg);
        break;

    case CCA_3:
        updateStatusCCA(event, msg);
        break;

    case TRANSMITFRAME_4:
        updateStatusTransmitFrame(event, msg);
        break;

    case WAITACK_5:
        updateStatusWaitAck(event, msg);
        break;

    case RECEIVEFRAME_6:
        updateStatusReceiveFrame(event, msg);
        break;

    case WAITSIFS_7:
        updateStatusSIFS(event, msg);
        break;

    case TRANSMITACK_8:
        updateStatusTransmitAck(event, msg);
        break;

    default:
        EV << "Error in TSCH FSM: an unknown state has been reached. macState="
                  << macState << endl;
        break;
    }
}

void Ieee802154eMac::updateMacState(t_mac_states newMacState) {
    macState = newMacState;
}

/*
 * Called by the FSM machine when an unknown transition is requested.
 */
void Ieee802154eMac::fsmError(t_mac_event event, cMessage *msg) {
    EV << "FSM Error ! In state " << macState << ", received unknown event:"
              << event << "." << endl;
    if (msg != nullptr)
        delete msg;
}

void Ieee802154eMac::startTimer(t_mac_timer timer) {
    if (timer == TIMER_SLOT) {
        currentAsn = asn.getAsn(simTime());
        auto next = schedule->getASNofNextLink(currentAsn);
        auto at = (next - currentAsn) * macTsTimeslotLength;

        EV_DETAIL << "(startTimer) slotTimer value=" << at << endl;
        scheduleAt(simTime() + at, slotTimer);
    } else if (timer == TIMER_CCA) {
        simtime_t ccaTime = rxSetupTime + ccaDetectionTime;
        EV_DETAIL << "(startTimer) ccaTimer value=" << ccaTime
                         << "(rxSetupTime,ccaDetectionTime:" << rxSetupTime
                         << "," << ccaDetectionTime << ")." << endl;
        scheduleAt(simTime() + rxSetupTime + ccaDetectionTime, ccaTimer);
    } else if (timer == TIMER_SIFS) {
        assert(useMACAcks);
        EV_DETAIL << "(startTimer) sifsTimer value=" << sifs << endl;
        scheduleAt(simTime() + sifs, sifsTimer);
    } else if (timer == TIMER_RX_ACK) {
        assert(useMACAcks);
        EV_DETAIL << "(startTimer) rxAckTimer value=" << macAckWaitDuration
                         << endl;
        scheduleAt(simTime() + macAckWaitDuration, rxAckTimer);
    } else if (timer == TIMER_HOPPING) {
        //assert(useMACAcks); TODO verify if channel hopping is enabled?
        // or always on and rely on TschHopping class to disable?
        EV_DETAIL << "(startTimer) hoppingTimer value=" << channelSwitchingTime
                         << endl;
        scheduleAt(simTime() + channelSwitchingTime, hoppingTimer);
    } else if (timer == TIMER_SLOTEND) {
        // TODO currently scheduled right before slot end,
        // but could be done more strict to increase radio sleep
        EV_DETAIL << "(startTimer) slotendTimer value="
                         << (macTsTimeslotLength * 0.75) << endl;
        scheduleAt(simTime() + (macTsTimeslotLength * 0.75), slotendTimer);
    } else {
        EV << "Unknown timer requested to start:" << timer << endl;
    }
}

/*
 * Binds timers to events and executes FSM.
 */
void Ieee802154eMac::handleSelfMessage(cMessage *msg) {
    EV_DETAIL << "timer routine." << endl;
    if (msg == slotTimer)
        executeMac(EV_TIMER_SLOT, msg);
    else if (msg == ccaTimer)
        executeMac(EV_TIMER_CCA, msg);
    else if (msg == sifsTimer)
        executeMac(EV_TIMER_SIFS, msg);
    else if (msg == hoppingTimer)
        executeMac(EV_TIMER_HOPPING, msg);
    else if (msg == slotendTimer)
        executeMac(EV_TIMER_SLOTEND, msg);
    else if (msg == rxAckTimer) {
        nbMissedAcks++;

        emitSignal(NBMISSEDACKS);

        executeMac(EV_ACK_TIMEOUT, msg);
    } else
        EV << "TSCH Error: unknown timer fired:" << msg << endl;
}

/**
 * Compares the address of this Host with the destination address in
 * frame. Generates the corresponding event.
 */
void Ieee802154eMac::handleLowerPacket(Packet *packet) {
    if (packet->hasBitError()) {
        EV << "Received " << packet
                  << " contains bit errors or collision, dropping it\n";
        PacketDropDetails details;
        details.setReason(INCORRECTLY_RECEIVED);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
        return;
    }
    const auto& csmaHeader = packet->peekAtFront<Ieee802154eMacHeader>();
    const MacAddress& src = csmaHeader->getSrcAddr();
    const MacAddress& dest = csmaHeader->getDestAddr();
    long ExpectedNr = 0;
    MacAddress address = interfaceEntry->getMacAddress();
    EV_DETAIL << "Received frame name= " << csmaHeader->getName()
                     << ", myState=" << macState << " src=" << src << " dst="
                     << dest << " myAddr=" << address << endl;
    if (dest == address) {
        if (!useMACAcks) {
            EV_DETAIL << "Received a data packet addressed to me." << endl;
            nbRxFrames++;

            emitSignal(NBRXFRAMES);

            EV << "Number of received frames: " << nbRxFrames << endl;
            executeMac(EV_FRAME_RECEIVED, packet);
        } else {
            long SeqNr = csmaHeader->getSequenceId();
            auto linkId = csmaHeader->getVirtualLinkID();

            if (strcmp(packet->getName(), "TSCH-Ack") != 0) {
                // This is a data message addressed to us
                // and we should send an ack.
                // we build the ack packet here because we need to
                // copy data from macPkt (src).
                EV_DETAIL << "Received a data packet addressed to me,"
                                 << " preparing an ack..." << endl;

                nbRxFrames++;

                emitSignal(NBRXFRAMES);

                EV << "Number of received frames: " << nbRxFrames << endl;
                if (ackMessage != nullptr)
                    delete ackMessage;
                auto csmaHeader = makeShared<Ieee802154eMacHeader>();
                csmaHeader->setSrcAddr(address);
                csmaHeader->setDestAddr(src);
                csmaHeader->setChunkLength(b(ackLength));
                ackMessage = new Packet("TSCH-Ack");
                ackMessage->insertAtFront(csmaHeader);
                ackMessage->addTag<PacketProtocolTag>()->setProtocol(
                        &Protocol::ieee802154);
                //Check for duplicates by checking expected seqNr of sender
                if (SeqNrChild.count(src) == 0) {
                    //no record of current child -> add expected next number to map
                    SeqNrChild[src][linkId] = SeqNr + 1;
                    EV_DETAIL
                                     << "Adding a new child to the map of Sequence numbers:"
                                     << src << " with linkId " << linkId << endl;
                    executeMac(EV_FRAME_RECEIVED, packet);
                } else {
                    if (SeqNrChild[src].count(linkId) == 0) {
                        //no record of current linkId -> add expected next number to map
                        SeqNrChild[src][linkId] = SeqNr + 1;
                        EV_DETAIL
                                         << "Adding a new linkId to the map of Sequence numbers:"
                                         << linkId << endl;
                        executeMac(EV_FRAME_RECEIVED, packet);
                    } else {
                        ExpectedNr = SeqNrChild[src][linkId];
                        EV_DETAIL << "Expected Sequence number is " << ExpectedNr
                                         << " and number of packet is " << SeqNr
                                         << " with linkid " << linkId << endl;
                        if (SeqNr < ExpectedNr) {
                            //Duplicate Packet, count and do not send to upper layer
                            nbDuplicates++;

                            emitSignal(NBDUPLICATES);

                            executeMac(EV_DUPLICATE_RECEIVED, packet);
                        } else {
                            SeqNrChild[src][linkId] = SeqNr + 1;
                            executeMac(EV_FRAME_RECEIVED, packet);
                        }
                    }
                }
            }
            else if (neighbor->getCurrentNeighborQueueSize() != 0) {
                // message is an ack and it is for us.
                // Is it from the right node ?
                Packet *firstPacket =
                        static_cast<Packet *>(neighbor->getCurrentNeighborQueueFirstPacket());
                const auto& csmaHeader = firstPacket->peekAtFront<
                        Ieee802154eMacHeader>();
                if (src == csmaHeader->getDestAddr()) {
                    nbRecvdAcks++;

                    emitSignal(NBRECVDACKS);

                    executeMac(EV_ACK_RECEIVED, packet);
                } else {
                    EV
                              << "Error! Received an ack from an unexpected source: src="
                              << src << ", I was expecting from node addr="
                              << csmaHeader->getDestAddr() << endl;
                    delete packet;
                }
            } else {
                EV
                          << "Error! Received an Ack while my send queue was empty. src="
                          << src << "." << endl;
                delete packet;
            }
        }
    } else if (dest.isBroadcast() || dest.isMulticast()) {
        executeMac(EV_BROADCAST_RECEIVED, packet);
    } else {
        EV_DETAIL << "packet not for me, deleting...\n";
        PacketDropDetails details;
        details.setReason(NOT_ADDRESSED_TO_US);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
    }
}

void Ieee802154eMac::receiveSignal(cComponent *source, simsignal_t signalID,
        long value, cObject *details) {
    Enter_Method_Silent();
    if (signalID == IRadio::transmissionStateChangedSignal) {
        IRadio::TransmissionState newRadioTransmissionState = static_cast<IRadio::TransmissionState>(value);
        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
            // KLUDGE: we used to get a cMessage from the radio (the identity was not important)
            executeMac(EV_FRAME_TRANSMITTED, new cMessage("Transmission over"));
        }
        transmissionState = newRadioTransmissionState;
    }
}

void Ieee802154eMac::decapsulate(Packet *packet) {
    const auto& tschHeader = packet->popAtFront<Ieee802154eMacHeader>();
    packet->addTagIfAbsent<MacAddressInd>()->setSrcAddress(
            tschHeader->getSrcAddr());
    packet->addTagIfAbsent<InterfaceInd>()->setInterfaceId(
            interfaceEntry->getInterfaceId());
    auto payloadProtocol = ProtocolGroup::ethertype.getProtocol(
            tschHeader->getNetworkProtocol());
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(payloadProtocol);
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(payloadProtocol);
    packet->addTagIfAbsent<VirtualLinkTagInd>()->setVirtualLinkID(
            tschHeader->getVirtualLinkID());

}

inet::physicallayer::IRadio* Ieee802154eMac::getRadio(){
    return this->radio;
}

void Ieee802154eMac::sendUp(cMessage *message)
{
    if (message->isPacket()) {
        emit(packetSentToUpperSignal, message);
    }
    if (message->isPacket()) {
        auto pkt = dynamic_cast<Packet*>(message);
        auto tag = pkt->getTag<PacketProtocolTag>();

        if (tag != nullptr && tag->getProtocol() == &Protocol::wiseRoute) {
            send(message, sixTopSublayerOutGateId);
            return;
        }
    }

    send(message, upperLayerOutGateId);
}


}
