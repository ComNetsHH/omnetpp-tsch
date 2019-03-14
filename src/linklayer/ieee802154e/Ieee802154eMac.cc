/* -*- mode:c++ -*- ********************************************************
 * file:        Ieee802154eMac.cc
 *
 * author:      Jerome Rousselot, Marcel Steine, Amre El-Hoiydi,
 *              Marc Loebbers, Yosia Hadisusanto, Andreas Koepke
 *
 * copyright:   (C) 2007-2009 CSEM SA
 *              (C) 2009 T.U. Eindhoven
 *                (C) 2004,2005,2006
 *              Telecommunication Networks Group (TKN) at Technische
 *              Universitaet Berlin, Germany.
 *
 *              This program is free software; you can redistribute it
 *              and/or modify it under the terms of the GNU General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later
 *              version.
 *              For further information see file COPYING
 *              in the top level directory
 *
 * Funding: This work was partially financed by the European Commission under the
 * Framework 6 IST Project "Wirelessly Accessible Sensor Populations"
 * (WASP) under contract IST-034963.
 ***************************************************************************
 * part of:    Modifications to the MF-2 framework by CSEM
 **************************************************************************/
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
#include "inet/linklayer/ieee802154/Ieee802154MacHeader_m.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/common/Units.h"
#include "inet/common/packet/Message.h"

namespace tsch {

using namespace inet::physicallayer;
using namespace omnetpp;
using namespace inet;

Define_Module(Ieee802154eMac);

void Ieee802154eMac::initialize(int stage)
{
    inet::MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        useMACAcks = par("useMACAcks");
        queueLength = par("queueLength");
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

        //init parameters for backoff method
        std::string backoffMethodStr = par("backoffMethod").stdstringValue();
        if (backoffMethodStr == "exponential") {
            backoffMethod = EXPONENTIAL;
            macMinBE = par("macMinBE");
            macMaxBE = par("macMaxBE");
        }
        else {
            throw cRuntimeError("Unknown backoff method \"%s\".\
                   Only \"exponential\", implemented \
                   so far.", backoffMethodStr.c_str());
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
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radio = check_and_cast<IRadio *>(radioModule);

        //check parameters for consistency
        //aTurnaroundTime should match (be equal or bigger) the RX to TX
        //switching time of the radio
        if (radioModule->hasPar("timeRXToTX")) {
            simtime_t rxToTx = radioModule->par("timeRXToTX");
            if (rxToTx > aTurnaroundTime) {
                throw cRuntimeError("Parameter \"aTurnaroundTime\" (%f) does not match"
                            " the radios RX to TX switching time (%f)! It"
                            " should be equal or bigger",
                        SIMTIME_DBL(aTurnaroundTime), SIMTIME_DBL(rxToTx));
            }
        }
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

        schedule = dynamic_cast<TschHopping*>(getModuleByPath("^.channelHopping"));

        // get our slotframe
        sf = dynamic_cast<TschSlotframe*>(getModuleByPath("^.schedule"));

        // minimal schedule as per 6TiSCH some document TODO
        TschLink *tl = sf->createLink();
        sf->addLink(tl);

//        tl = sf->createLink();
//        tl->setSlotOffset(25);
//        sf->addLink(tl);

        tl = sf->createLink();
        tl->setSlotOffset(50);
        sf->addLink(tl);

        sf->printSlotframe();



        asn.setMacTsTimeslotLength(macTsTimeslotLength);
        asn.setReference(simTime() + 0.1);
        scheduleAt(simTime() + 0.1, slotTimer);

        EV_DETAIL << "queueLength = " << queueLength
                  << " bitrate = " << bitrate
                  << " backoff method = " << par("backoffMethod").stringValue() << endl;

        EV_DETAIL << "finished tsch init stage 1." << endl;
    }
}

void Ieee802154eMac::finish()
{
    recordScalar("nbTxFrames", nbTxFrames);
    recordScalar("nbRxFrames", nbRxFrames);
    recordScalar("nbDroppedFrames", nbDroppedFrames);
    recordScalar("nbMissedAcks", nbMissedAcks);
    recordScalar("nbRecvdAcks", nbRecvdAcks);
    recordScalar("nbTxAcks", nbTxAcks);
    recordScalar("nbDuplicates", nbDuplicates);
    if (nbBackoffs > 0) {
        recordScalar("meanBackoff", backoffValues / nbBackoffs);
    }
    else {
        recordScalar("meanBackoff", 0);
    }
    recordScalar("nbBackoffs", nbBackoffs);
    recordScalar("backoffDurations", backoffValues);
}

Ieee802154eMac::~Ieee802154eMac()
{
    cancelAndDelete(slotTimer);
    cancelAndDelete(ccaTimer);
    cancelAndDelete(sifsTimer);
    cancelAndDelete(rxAckTimer);
    cancelAndDelete(slotendTimer);
    cancelAndDelete(hoppingTimer);
    if (ackMessage)
        delete ackMessage;
    for (auto & elem : macQueue) {
        delete (elem);
    }
}

void Ieee802154eMac::configureInterfaceEntry()
{
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

/**
 * Encapsulates the message to be transmitted and pass it on
 * to the FSM main method for further processing.
 */
void Ieee802154eMac::handleUpperPacket(Packet *packet)
{
    //MacPkt*macPkt = encapsMsg(msg);
    auto macPkt = makeShared<Ieee802154MacHeader>();
    assert(headerLength % 8 == 0);
    macPkt->setChunkLength(b(headerLength));
    MacAddress dest = packet->getTag<MacAddressReq>()->getDestAddress();
    EV_DETAIL << "TSCH received a message from upper layer, name is " << packet->getName() << ", CInfo removed, mac addr=" << dest << endl;
    macPkt->setNetworkProtocol(ProtocolGroup::ethertype.getProtocolNumber(packet->getTag<PacketProtocolTag>()->getProtocol()));
    macPkt->setDestAddr(dest);
    delete packet->removeControlInfo();
    macPkt->setSrcAddr(interfaceEntry->getMacAddress());

    if (useMACAcks) {
        if (SeqNrParent.find(dest) == SeqNrParent.end()) {
            //no record of current parent -> add next sequence number to map
            SeqNrParent[dest] = 1;
            macPkt->setSequenceId(0);
            EV_DETAIL << "Adding a new parent to the map of Sequence numbers:" << dest << endl;
        }
        else {
            macPkt->setSequenceId(SeqNrParent[dest]);
            EV_DETAIL << "Packet send with sequence number = " << SeqNrParent[dest] << endl;
            SeqNrParent[dest]++;
        }
    }

    //RadioAccNoise3PhyControlInfo *pco = new RadioAccNoise3PhyControlInfo(bitrate);
    //macPkt->setControlInfo(pco);
    packet->insertAtFront(macPkt);
    // TODO we are using protocol ieee802154 for now
    packet->getTag<PacketProtocolTag>()->setProtocol(&Protocol::ieee802154);
    EV_DETAIL << "pkt encapsulated, length: " << macPkt->getChunkLength() << "\n";
    macQueue.push_back(packet);
    //executeMac(EV_SEND_REQUEST, packet);
}

void Ieee802154eMac::updateStatusIdle(t_mac_event event, cMessage *msg)
{
    switch (event) {
        case EV_TIMER_SLOT: {
            EV_DETAIL << "(1) FSM State IDLE_1, EV_TIMER_SLOT: startTimerSlot -> idle." << endl;
            // directly schedule next slot
            startTimer(TIMER_SLOT);

            // start of new slot
            auto now = asn.getAsn(simTime());
            auto link = sf->getLinkFromASN(now);
            auto chan = schedule->channel(now, link->getChannelOffset());
            auto freq = schedule->frequency(chan);

            EV_DETAIL << link->str() << endl;
            EV_DETAIL << "we are in ASN " << now << " queue size " << macQueue.size() << " channel " << chan << " frequency " << freq << endl;

            // TODO per neighbor queue necessary
            if (macQueue.size() > 0 && link->isTx()) {
                updateMacState(CCA_3);
                startTimer(TIMER_CCA);

                configureRadio(freq, IRadio::RADIO_MODE_RECEIVER);

            } else if (link->isRx()) {
                updateMacState(RECEIVEFRAME_6);
                startTimer(TIMER_SLOTEND);

                configureRadio(freq, IRadio::RADIO_MODE_RECEIVER);
            }
            break;
        }
        default:
            fsmError(event, msg);
            break;
    }

//        case EV_DUPLICATE_RECEIVED:
//            EV_DETAIL << "(15) FSM State IDLE_1, EV_DUPLICATE_RECEIVED: setting up radio tx -> WAITSIFS." << endl;
//            //sendUp(decapsMsg(static_cast<MacSeqPkt *>(msg)));
//            delete msg;
//
//            if (useMACAcks) {
//                radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
//                updateMacState(WAITSIFS_7);
//                startTimer(TIMER_SIFS);
//            }
//            break;
//
//        case EV_FRAME_RECEIVED:
//            EV_DETAIL << "(15) FSM State IDLE_1, EV_FRAME_RECEIVED: setting up radio tx -> WAITSIFS." << endl;
//            decapsulate(check_and_cast<Packet *>(msg));
//            sendUp(msg);
//            nbRxFrames++;
//
//            if (useMACAcks) {
//                radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
//                updateMacState(WAITSIFS_7);
//                startTimer(TIMER_SIFS);
//            }
//            break;
//
//        case EV_BROADCAST_RECEIVED:
//            EV_DETAIL << "(23) FSM State IDLE_1, EV_BROADCAST_RECEIVED: Nothing to do." << endl;
//            nbRxFrames++;
//            decapsulate(check_and_cast<Packet *>(msg));
//            sendUp(msg);
//            break;

}

void Ieee802154eMac::configureRadio(double carrierFrequency /*= NAN*/, int mode /*= -1*/)
{
    auto configureCommand = new ConfigureRadioCommand();
    auto request = new Message("changeChannel", RADIO_C_CONFIGURE);

    configureCommand->setCarrierFrequency(Hz(carrierFrequency));
    configureCommand->setRadioMode(mode);
    request->setControlInfo(configureCommand);

    sendDown(request);
}

void Ieee802154eMac::flushQueue()
{
    // TODO:
    macQueue.clear();
}

void Ieee802154eMac::clearQueue()
{
    macQueue.clear();
}

void Ieee802154eMac::attachSignal(Packet *mac, simtime_t_cref startTime)
{
    simtime_t duration = mac->getBitLength() / bitrate;
    mac->setDuration(duration);
}

void Ieee802154eMac::updateStatusCCA(t_mac_event event, cMessage *msg)
{
    switch (event) {
        case EV_TIMER_CCA: {
            EV_DETAIL << "(25) FSM State CCA_3, EV_TIMER_CCA" << endl;
            bool isIdle = radio->getReceptionState() == IRadio::RECEPTION_STATE_IDLE;
            if (isIdle) {
                EV_DETAIL << "(3) FSM State CCA_3, EV_TIMER_CCA, [Channel Idle]: -> TRANSMITFRAME_4." << endl;
                updateMacState(TRANSMITFRAME_4);
                radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
                Packet *mac = check_and_cast<Packet *>(macQueue.front()->dup());
                attachSignal(mac, simTime() + aTurnaroundTime);
                //sendDown(msg);
                // give time for the radio to be in Tx state before transmitting
                sendDelayed(mac, aTurnaroundTime, lowerLayerOutGateId);
                nbTxFrames++;
            }
            else {
                // Channel was busy, increment 802.15.4 backoff timers as specified.
                EV_DETAIL << "(7) FSM State CCA_3, EV_TIMER_CCA, [Channel Busy]: "
                          << " skipping slot." << endl;

                radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                updateMacState(IDLE_1);
            }
            break;
        }

//        case EV_DUPLICATE_RECEIVED:
//            EV_DETAIL << "(26) FSM State CCA_3, EV_DUPLICATE_RECEIVED:";
//            if (useMACAcks) {
//                EV_DETAIL << " setting up radio tx -> WAITSIFS." << endl;
//                // suspend current transmission attempt,
//                // transmit ack,
//                // and resume transmission when entering manageQueue()
//                transmissionAttemptInterruptedByRx = true;
//                cancelEvent(ccaTimer);
//
//                radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
//                updateMacState(WAITSIFS_7);
//                startTimer(TIMER_SIFS);
//            }
//            else {
//                EV_DETAIL << " Nothing to do." << endl;
//            }
//            //sendUp(decapsMsg(static_cast<MacPkt*>(msg)));
//            delete msg;
//            break;
//
//        case EV_FRAME_RECEIVED:
//            EV_DETAIL << "(26) FSM State CCA_3, EV_FRAME_RECEIVED:";
//            if (useMACAcks) {
//                EV_DETAIL << " setting up radio tx -> WAITSIFS." << endl;
//                // suspend current transmission attempt,
//                // transmit ack,
//                // and resume transmission when entering manageQueue()
//                transmissionAttemptInterruptedByRx = true;
//                cancelEvent(ccaTimer);
//                radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
//                updateMacState(WAITSIFS_7);
//                startTimer(TIMER_SIFS);
//            }
//            else {
//                EV_DETAIL << " Nothing to do." << endl;
//            }
//            decapsulate(check_and_cast<Packet *>(msg));
//            sendUp(msg);
//            break;
//
//        case EV_BROADCAST_RECEIVED:
//            EV_DETAIL << "(24) FSM State BACKOFF, EV_BROADCAST_RECEIVED:"
//                      << " Nothing to do." << endl;
//            decapsulate(check_and_cast<Packet *>(msg));
//            sendUp(msg);
//            break;

        default:
            fsmError(event, msg);
            break;
    }
}

void Ieee802154eMac::updateStatusTransmitFrame(t_mac_event event, cMessage *msg)
{
    if (event == EV_FRAME_TRANSMITTED) {
        //    delete msg;
        Packet *packet = macQueue.front();
        const auto& csmaHeader = packet->peekAtFront<Ieee802154MacHeader>();

        bool expectAck = useMACAcks;
        if (!csmaHeader->getDestAddr().isBroadcast() && !csmaHeader->getDestAddr().isMulticast()) {
            //unicast
            EV_DETAIL << "(4) FSM State TRANSMITFRAME_4, "
                      << "EV_FRAME_TRANSMITTED [Unicast]: ";
        }
        else {
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
        }
        else {
            EV_DETAIL << ": RadioSetupSleep, manageQueue..." << endl;
            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            macQueue.pop_front();
            delete packet;
            manageQueue();
            updateMacState(IDLE_1);
        }
        delete msg;
    }
    else {
        fsmError(event, msg);
    }
}

void Ieee802154eMac::updateStatusWaitAck(t_mac_event event, cMessage *msg)
{
    assert(useMACAcks);

    switch (event) {
        case EV_ACK_RECEIVED: {
            EV_DETAIL << "(5) FSM State WAITACK_5, EV_ACK_RECEIVED: "
                      << " ProcessAck, manageQueue..." << endl;
            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            if (rxAckTimer->isScheduled())
                cancelEvent(rxAckTimer);
            cMessage *mac = macQueue.front();
            macQueue.pop_front();
            txAttempts = 0;
            delete mac;
            delete msg;
            manageQueue();
            updateMacState(IDLE_1);
            break;
        }
        case EV_ACK_TIMEOUT:
            EV_DETAIL << "(12) FSM State WAITACK_5, EV_ACK_TIMEOUT:"
                      << " incrementCounter/dropPacket, manageQueue..." << endl;
            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            manageMissingAck(event, msg);
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

void Ieee802154eMac::manageMissingAck(t_mac_event    /*event*/, cMessage *    /*msg*/)
{
    if (txAttempts < macMaxFrameRetries) {
        // increment counter
        txAttempts++;
        EV_DETAIL << "I will retransmit this packet (I already tried "
                  << txAttempts << " times)." << endl;
    }
    else {
        // drop packet
        EV_DETAIL << "Packet was transmitted " << txAttempts
                  << " times and I never got an Ack. I drop the packet." << endl;
        cMessage *mac = macQueue.front();
        macQueue.pop_front();
        txAttempts = 0;
        PacketDropDetails details;
        details.setReason(RETRY_LIMIT_REACHED);
        details.setLimit(macMaxFrameRetries);
        emit(packetDroppedSignal, mac, &details);
        emit(linkBrokenSignal, mac);
        delete mac;
    }
    manageQueue();
}

void Ieee802154eMac::updateStatusReceiveFrame(t_mac_event event, omnetpp::cMessage *msg)
{
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
            }
            else {
                EV_DETAIL << " going to sleep" << endl;

                radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                updateMacState(IDLE_1);
            }
            //sendUp(decapsMsg(static_cast<MacPkt*>(msg)));
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
                // and resume transmission when entering manageQueue()

                radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
                updateMacState(WAITSIFS_7);
                startTimer(TIMER_SIFS);
            }
            else {
                EV_DETAIL << " Nothing to do." << endl;

                radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                updateMacState(IDLE_1);
            }
            decapsulate(check_and_cast<Packet *>(msg));
            sendUp(msg);
            break;

        case EV_BROADCAST_RECEIVED:
            EV_DETAIL << "(24) FSM State RECEIVEFRAME_6, EV_BROADCAST_RECEIVED:"
                      << " Nothing to do." << endl;

            if (slotendTimer->isScheduled())
                cancelEvent(slotendTimer);

            decapsulate(check_and_cast<Packet *>(msg));
            sendUp(msg);
            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            updateMacState(IDLE_1);
            break;

        case EV_TIMER_SLOTEND:
            EV_DETAIL << "(24) FSM State RECEIVEFRAME_6, EV_TIMER_SLOTEND:"
                      << " Nothing received within slot" << endl;
            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            updateMacState(IDLE_1);
            break;

        default:
            fsmError(event, msg);
            break;
    }
}

void Ieee802154eMac::updateStatusSIFS(t_mac_event event, cMessage *msg)
{
    assert(useMACAcks);

    switch (event) {
        case EV_TIMER_SIFS:
            EV_DETAIL << "(17) FSM State WAITSIFS_7, EV_TIMER_SIFS:"
                      << " sendAck -> TRANSMITACK." << endl;
            updateMacState(TRANSMITACK_8);
            attachSignal(ackMessage, simTime());
            sendDown(ackMessage);
            nbTxAcks++;
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

void Ieee802154eMac::updateStatusTransmitAck(t_mac_event event, cMessage *msg)
{
    assert(useMACAcks);

    if (event == EV_FRAME_TRANSMITTED) {
        EV_DETAIL << "(19) FSM State TRANSMITACK_8, EV_FRAME_TRANSMITTED:"
                  << " ->manageQueue." << endl;
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        delete msg;
        manageQueue();
        updateMacState(IDLE_1);
    }
    else {
        fsmError(event, msg);
    }
}

/**
 * Updates state machine.
 */
void Ieee802154eMac::executeMac(t_mac_event event, cMessage *msg)
{
    EV_DETAIL << "In executeMac" << endl;

//    if (macState != IDLE_1 && event == EV_SEND_REQUEST) {
//        updateStatusNotIdle(msg);
//    }
//    else {
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
            EV << "Error in TSCH FSM: an unknown state has been reached. macState=" << macState << endl;
            break;
    }
//    }
}

void Ieee802154eMac::manageQueue()
{
    if (macQueue.size() != 0) {
        EV_DETAIL << "(manageQueue) there are " << macQueue.size() << " packets to send, entering backoff wait state." << endl;
        if (transmissionAttemptInterruptedByRx) {
            // resume a transmission cycle which was interrupted by
            // a frame reception during CCA check
            transmissionAttemptInterruptedByRx = false;
        }
        else {
            // initialize counters if we start a new transmission
            // cycle from zero
            NB = 0;
            //BE = macMinBE;
        }
//        if (!slotTimer->isScheduled()) {
//            startTimer(TIMER_SLOT);
//        }
//        updateMacState(BACKOFF_2);
    }
    else {
        EV_DETAIL << "(manageQueue) no packets to send, entering IDLE state." << endl;
//        updateMacState(IDLE_1);
    }
}

void Ieee802154eMac::updateMacState(t_mac_states newMacState)
{
    macState = newMacState;
}

/*
 * Called by the FSM machine when an unknown transition is requested.
 */
void Ieee802154eMac::fsmError(t_mac_event event, cMessage *msg)
{
    EV << "FSM Error ! In state " << macState << ", received unknown event:" << event << "." << endl;
    if (msg != nullptr)
        delete msg;
}

void Ieee802154eMac::startTimer(t_mac_timer timer)
{
    if (timer == TIMER_SLOT) {
        auto now  = asn.getAsn(simTime());
        auto next = sf->getASNofNextLink(now);
        auto at = (next - now) * macTsTimeslotLength;

        EV_DETAIL << "(startTimer) slotTimer value=" << at << endl;
        scheduleAt(simTime() + at, slotTimer);
    }
    else if (timer == TIMER_CCA) {
        simtime_t ccaTime = rxSetupTime + ccaDetectionTime;
        EV_DETAIL << "(startTimer) ccaTimer value=" << ccaTime
                  << "(rxSetupTime,ccaDetectionTime:" << rxSetupTime
                  << "," << ccaDetectionTime << ")." << endl;
        scheduleAt(simTime() + rxSetupTime + ccaDetectionTime, ccaTimer);
    }
    else if (timer == TIMER_SIFS) {
        assert(useMACAcks);
        EV_DETAIL << "(startTimer) sifsTimer value=" << sifs << endl;
        scheduleAt(simTime() + sifs, sifsTimer);
    }
    else if (timer == TIMER_RX_ACK) {
        assert(useMACAcks);
        EV_DETAIL << "(startTimer) rxAckTimer value=" << macAckWaitDuration << endl;
        scheduleAt(simTime() + macAckWaitDuration, rxAckTimer);
    }
    else if (timer == TIMER_HOPPING) {
        //assert(useMACAcks); TODO verify if channel hopping is enabled?
        // or always on and rely on TschHopping class to disable?
        EV_DETAIL << "(startTimer) hoppingTimer value=" << channelSwitchingTime << endl;
        scheduleAt(simTime() + channelSwitchingTime, hoppingTimer);
    }
    else if (timer == TIMER_SLOTEND) {
        // TODO currently scheduled right before slot end,
        // but could be done more strict to increase radio sleep
        EV_DETAIL << "(startTimer) slotendTimer value=" << (macTsTimeslotLength * 0.75) << endl;
        scheduleAt(simTime() + (macTsTimeslotLength*0.75), slotendTimer);
    }
    else {
        EV << "Unknown timer requested to start:" << timer << endl;
    }
}

/*
 * Binds timers to events and executes FSM.
 */
void Ieee802154eMac::handleSelfMessage(cMessage *msg)
{
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
        nbMissedAcks++; // TODO collect per link
        executeMac(EV_ACK_TIMEOUT, msg);
    }
    else
        EV << "TSCH Error: unknown timer fired:" << msg << endl;
}

/**
 * Compares the address of this Host with the destination address in
 * frame. Generates the corresponding event.
 */
void Ieee802154eMac::handleLowerPacket(Packet *packet)
{
    if (packet->hasBitError()) {
        EV << "Received " << packet << " contains bit errors or collision, dropping it\n";
        PacketDropDetails details;
        details.setReason(INCORRECTLY_RECEIVED);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
        return;
    }
    const auto& csmaHeader = packet->peekAtFront<Ieee802154MacHeader>();
    const MacAddress& src = csmaHeader->getSrcAddr();
    const MacAddress& dest = csmaHeader->getDestAddr();
    long ExpectedNr = 0;
    MacAddress address = interfaceEntry->getMacAddress();

    EV_DETAIL << "Received frame name= " << csmaHeader->getName()
              << ", myState=" << macState << " src=" << src
              << " dst=" << dest << " myAddr="
              << address << endl;

    if (dest == address) {
        if (!useMACAcks) {
            EV_DETAIL << "Received a data packet addressed to me." << endl;
//            nbRxFrames++;
            executeMac(EV_FRAME_RECEIVED, packet);
        }
        else {
            long SeqNr = csmaHeader->getSequenceId();

            if (strcmp(packet->getName(), "TSCH-Ack") != 0) {
                // This is a data message addressed to us
                // and we should send an ack.
                // we build the ack packet here because we need to
                // copy data from macPkt (src).
                EV_DETAIL << "Received a data packet addressed to me,"
                          << " preparing an ack..." << endl;

//                nbRxFrames++;

                if (ackMessage != nullptr)
                    delete ackMessage;
                auto csmaHeader = makeShared<Ieee802154MacHeader>();
                csmaHeader->setSrcAddr(address);
                csmaHeader->setDestAddr(src);
                csmaHeader->setChunkLength(b(ackLength));
                ackMessage = new Packet("TSCH-Ack");
                ackMessage->insertAtFront(csmaHeader);
                ackMessage->addTag<PacketProtocolTag>()->setProtocol(&Protocol::ieee802154);
                //Check for duplicates by checking expected seqNr of sender
                if (SeqNrChild.find(src) == SeqNrChild.end()) {
                    //no record of current child -> add expected next number to map
                    SeqNrChild[src] = SeqNr + 1;
                    EV_DETAIL << "Adding a new child to the map of Sequence numbers:" << src << endl;
                    executeMac(EV_FRAME_RECEIVED, packet);
                }
                else {
                    ExpectedNr = SeqNrChild[src];
                    EV_DETAIL << "Expected Sequence number is " << ExpectedNr
                              << " and number of packet is " << SeqNr << endl;
                    if (SeqNr < ExpectedNr) {
                        //Duplicate Packet, count and do not send to upper layer
                        nbDuplicates++;
                        executeMac(EV_DUPLICATE_RECEIVED, packet);
                    }
                    else {
                        SeqNrChild[src] = SeqNr + 1;
                        executeMac(EV_FRAME_RECEIVED, packet);
                    }
                }
            }
            else if (macQueue.size() != 0) {
                // message is an ack, and it is for us.
                // Is it from the right node ?
                Packet *firstPacket = static_cast<Packet *>(macQueue.front());
                const auto& csmaHeader = firstPacket->peekAtFront<Ieee802154MacHeader>();
                if (src == csmaHeader->getDestAddr()) {
                    nbRecvdAcks++;
                    executeMac(EV_ACK_RECEIVED, packet);
                }
                else {
                    EV << "Error! Received an ack from an unexpected source: src=" << src << ", I was expecting from node addr=" << csmaHeader->getDestAddr() << endl;
                    delete packet;
                }
            }
            else {
                EV << "Error! Received an Ack while my send queue was empty. src=" << src << "." << endl;
                delete packet;
            }
        }
    }
    else if (dest.isBroadcast() || dest.isMulticast()) {
        executeMac(EV_BROADCAST_RECEIVED, packet);
    }
    else {
        EV_DETAIL << "packet not for me, deleting...\n";
        PacketDropDetails details;
        details.setReason(NOT_ADDRESSED_TO_US);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
    }
}

void Ieee802154eMac::receiveSignal(cComponent *source, simsignal_t signalID, long value, cObject *details)
{
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

void Ieee802154eMac::decapsulate(Packet *packet)
{
    const auto& tschHeader = packet->popAtFront<Ieee802154MacHeader>();
    packet->addTagIfAbsent<MacAddressInd>()->setSrcAddress(tschHeader->getSrcAddr());
    packet->addTagIfAbsent<InterfaceInd>()->setInterfaceId(interfaceEntry->getInterfaceId());
    auto payloadProtocol = ProtocolGroup::ethertype.getProtocol(tschHeader->getNetworkProtocol());
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(payloadProtocol);
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(payloadProtocol);
}
}
