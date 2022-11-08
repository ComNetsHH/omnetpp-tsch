/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright   (C) 2021  Institute of Communication Networks (ComNets),
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
#include "TschVirtualLink.h"
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
        macTsTxAckDelay = par("macTsTxAckDelay");
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
        //macAckWaitDuration = par("macAckWaitDuration");
        macTsRxAckDelay = par("macTsRxAckDelay");
        macTsAckWait = par("macTsAckWait");
        macTsMaxAck = par("macTsMaxAck");
        ccaDetectionTime = par("ccaDetectionTime");
        useCCA = par("useCCA");
        rxSetupTime = par("rxSetupTime");
        macTsRxTx = par("macTsRxTx");
        channelSwitchingTime = par("channelSwitchingTime");
        bitrate = par("bitrate");
        ackLength = par("ackLength");
        ackMessage = nullptr;
        currentAsn = 0;
        currentLink = nullptr;
        currentChannel = 0;

        w_np = par("wrrWeigthNp").intValue();
        w_be = par("wrrWeigthBe").intValue();
        ignoreBitErrors = par("ignoreBitErrors").boolValue();
        isSink = getModuleByPath("^.^.^.rpl")->par("isRoot").boolValue();

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
        //macTsRxTx should match (be equal or bigger) the RX to TX
        //switching time of the radio
        if (radioModule->hasPar("timeRXToTX")) {
            simtime_t rxToTx = radioModule->par("timeRXToTX");
            if (rxToTx > macTsRxTx) {
                throw cRuntimeError(
                        "Parameter \"macTsRxTx\" (%f) does not match"
                                " the radios RX to TX switching time (%f)! It"
                                " should be equal or bigger",
                        SIMTIME_DBL(macTsRxTx), SIMTIME_DBL(rxToTx));
            }
        }
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

        hopping = dynamic_cast<TschHopping*>(getModuleByPath("^.^.^.^.channelHopping"));
        if (!hopping)
            throw cRuntimeError("channelHopping module not found");

        schedule = dynamic_cast<TschSlotframe*>(getModuleByPath("^.schedule"));
        if (!schedule)
            throw cRuntimeError("schedule module not found");

        neighbor = dynamic_cast<TschNeighbor*>(getModuleByPath("^.neighbor"));
        if (!hopping)
            throw cRuntimeError("neighbor module not found");

        // Use XML schedule only if SF is disabled
        sf = check_and_cast<TschSF*> (getModuleByPath("^.sixtischInterface.sf"));
        if (sf->par("disable").boolValue()) {
            schedule->xmlSchedule();
//            schedule->printSlotframe();
        }

        asn.setMacTsTimeslotLength(macTsTimeslotLength);
        asn.setReference(simTime() + 0.1);
        scheduleAt(simTime() + 0.1, slotTimer);

        EV_DETAIL << "QueueLength = " << neighbor->getQueueLength() << " bitrate = " << bitrate << endl;
        EV_DETAIL << "Finished tsch init stage 1." << endl;

        pktRetransmittedSignal = registerSignal("pktRetransmitted");
        pktRetransmittedDownlinkSignal = registerSignal("pktRetransmittedDownlink");
        pktInterarrivalTimeSignal = registerSignal("interarrivalTime");
        pktRecFromUpperSignal = registerSignal("pktReceviedFromUpperLayer");
        pktRecFromLowerSignal = registerSignal("pktReceviedFromLowerLayer");
        currentFreqSignal = registerSignal("currentFrequency");
    } else if (stage == INITSTAGE_LAST) {
        WATCH_MAP(packetsIncorrectlyReceived);

        auto timeoutVal = par("lossyLinkTimeout").doubleValue();
        if (timeoutVal)
            scheduleAt(simTime() + SimTime(timeoutVal, SIMTIME_S), new cMessage("Start losing packets", MAC_ENABLE_DROPS));
    }
}

void Ieee802154eMac::finish() {
//    recordScalar("nbTxFrames", nbTxFrames);
//    recordScalar("nbRxFrames", nbRxFrames);
//    recordScalar("nbDroppedFrames", nbDroppedFrames);
//    recordScalar("nbMissedAcks", nbMissedAcks);
//    recordScalar("nbRecvdAcks", nbRecvdAcks);
//    recordScalar("nbTxAcks", nbTxAcks);
//    recordScalar("nbDuplicates", nbDuplicates);
//    if (nbBackoffs > 0) {
//        recordScalar("meanBackoff", backoffValues / nbBackoffs);
//    } else {
//        recordScalar("meanBackoff", 0);
//    }
//    recordScalar("nbBackoffs", nbBackoffs);
//    recordScalar("backoffDurations", backoffValues);
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

bool Ieee802154eMac::isControlPacket(Packet *packet) {
    std::string packetName(packet->getFullName());

    // FIXME: make a list of control packet names a constant somewhere
    return packetName.find("NA") != std::string::npos
            || packetName.find("NS") != std::string::npos;
}

/**
 * Encapsulates the message to be transmitted and pass it on
 * to the FSM main method for further processing.
 */
void Ieee802154eMac::handleUpperPacket(Packet *packet) {
    //MacPkt*macPkt = encapsMsg(msg);
    auto macPkt = makeShared<Ieee802154eMacHeader>();
    auto proto = packet->getTag<PacketProtocolTag>()->getProtocol();

    auto virtualLinkTagReq = packet->findTag<VirtualLinkTagReq>();
    auto virtualLinkTagInd = packet->findTag<VirtualLinkTagInd>();

    int linkId = 0;

    if (virtualLinkTagReq) {
        linkId = virtualLinkTagReq->getVirtualLinkID();
        EV_DETAIL << "virtual link ID set from tag REQ = " << linkId << endl;
    }
    else if (virtualLinkTagInd) {
        linkId = virtualLinkTagInd->getVirtualLinkID();
        EV_DETAIL << "virtual link ID set from tag IND = " << linkId << endl;
    }

    if (isControlPacket(packet))
        linkId = LINK_PRIO_CONTROL;

    macPkt->setVirtualLinkID(linkId);
    assert(headerLength % 8 == 0);
    macPkt->setChunkLength(b(headerLength));
    MacAddress dest = packet->getTag<MacAddressReq>()->getDestAddress();
    EV_DETAIL << "TSCH received a message from upper layer, name is "
             << packet->getName() << ", CInfo removed, mac addr = " << dest << endl;
    macPkt->setNetworkProtocol(ProtocolGroup::ethertype.getProtocolNumber(proto));
    macPkt->setDestAddr(dest);
    delete packet->removeControlInfo();
    macPkt->setSrcAddr(interfaceEntry->getMacAddress());

    if (useMACAcks)
    {
        if (SeqNrParent.count(dest) == 0) {
            //no record of current parent -> add next sequence number to map
            SeqNrParent[dest][linkId] = 1;
            macPkt->setSequenceId(0);
            EV_DETAIL << "Adding a new parent to the sequence numbers map:"
                     << dest << " with link ID = " << linkId << endl;
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
    EV_DETAIL << "Pkt encapsulated, length: " << macPkt->getChunkLength() << endl;

    // Notify scheduling function (MSF) to ensure there's a cell available for transmission
    auto ctrlInfo = new MacGenericInfo(dest.getInt());
    emit(pktRecFromUpperSignal, 0, (cObject*) ctrlInfo);

    if (neighbor->add2Queue(packet, dest, linkId)) {
        EV_DETAIL << "Added packet to queue with link ID " << linkId << endl;

        if (!isAppPacket(packet))
            return;

        if (lastAppPktArrivalTimestamp > 0)
            emit(pktInterarrivalTimeSignal, simTime().dbl() - lastAppPktArrivalTimestamp);

        lastAppPktArrivalTimestamp = simTime().dbl();
    } else {
        EV_DETAIL << "Packet is dropped due to Queue Overflow" << endl;
        PacketDropDetails details;
        details.setReason(QUEUE_OVERFLOW);
        details.setLimit(neighbor->getQueueLength());
        emit(packetDroppedSignal, packet, &details);
        delete packet;
    }
}

bool Ieee802154eMac::isAppPacket(Packet *packet) {
    std::string packetName(packet->getFullName());

    return packetName.find("App") != std::string::npos;
}

bool Ieee802154eMac::isSmokeAlarmPacket(Packet *packet) {
    std::string packetName(packet->getFullName());

    return packetName.find("HAZARD") != std::string::npos;
}

int Ieee802154eMac::getVirtualLinkId(TschLink* link) {
    auto vl = dynamic_cast<TschVirtualLink*>(link);

    return vl ? vl->getVirtualLink() : 0;
}

vector<tuple<int, int>> Ieee802154eMac::getQueueSizes(MacAddress nbrAddr, vector<int> virtuaLinkIds)
{
    vector<tuple<int, int>> queueSizes;

    for (auto linkId : virtuaLinkIds)
        queueSizes.emplace_back(linkId, neighbor->getVirtualQueueSizeAt(nbrAddr, linkId));

    return queueSizes;
}

int Ieee802154eMac::getQueueSize(MacAddress nbrAddr) {
    return neighbor->getTotalQueueSizeAt(nbrAddr);
}

double Ieee802154eMac::getQueueUtilization(MacAddress nbrAddr) {
    return neighbor->getTotalQueueSizeAt(nbrAddr) / (double) neighbor->getQueueLength();
}

double Ieee802154eMac::getQueueUtilization(MacAddress nbrAddr, int virtualLinkId) {
    auto numEnqPackets = std::get<1>(getQueueSizes(nbrAddr, {virtualLinkId}).back());

    return numEnqPackets / (double) neighbor->getQueueLength();
}

TschLink* Ieee802154eMac::selectActiveLink(std::vector<TschLink*> links) {
    if (!links.size())
        return nullptr;

    if ((int) links.size() == 1)
        return links.back();

    // If multiple links / cells are scheduled at the same position
    // First look for unicast TX link with packets in the queue
    std::vector<TschLink*> unicastTxLinks = {};
    for (auto link : links)
    {
        if (link->isTx() && link->getAddr() != MacAddress::BROADCAST_ADDRESS
                && neighbor->getTotalQueueSizeAt(link->getAddr()) > 0)
        {
            unicastTxLinks.push_back(link);
        }

        if (link->isTx() && !link->isShared())
            // Update the elapsed counter also for inactive links! (but not shared)
            // TODO: turns the following code really ugly with multiple decrements, find a leaner solution
            sf->incrementNeighborCellElapsed(link->getAddr().getInt());
    }

    if ((int) unicastTxLinks.size() == 1) {
        EV_DETAIL << "Found unicast TX link with non-empty queue: " << unicastTxLinks.back()->str() << endl;

        // Compensate for the "catch-all" increment above, since active link is handled accounted for also by the SF
        sf->decrementNeighborCellElapsed(unicastTxLinks.back()->getAddr().getInt());
        return unicastTxLinks.back();
    }
    else if ((int) unicastTxLinks.size() > 1) {
        EV_DETAIL << "Multiple unicast links with non-empty queues found, selecting one randomly: " << endl;

        auto selectedId = intrand((int) unicastTxLinks.size());
        auto selectedLink = unicastTxLinks[selectedId];

        sf->decrementNeighborCellElapsed(selectedLink->getAddr().getInt());

        return selectedLink;
    }

    // Second, check if there's any other TX link: shared, broadcast with packets in queue
    for (auto link : links)
        if (link->isTx() && neighbor->getTotalQueueSizeAt(link->getAddr()) > 0)
        {
            EV_DETAIL << "Found shared TX link with non-empty queue: " << link->str() << endl;

            sf->decrementNeighborCellElapsed(link->getAddr().getInt());
            return link;
        }

    EV_DETAIL << "No active TX link detected, looking for RX" << endl;

    // Else just pick remaining RX/AUTO link randomly
    std::vector<TschLink*> rxLinks = {};
    for (auto link : links)
        if (link->isRx())
            rxLinks.push_back(link);

    if (!rxLinks.size()) {
        EV_DETAIL << "No RX links found" << endl;
        return nullptr;
    }

    TschLink* activeRxLink;

    if ((int) rxLinks.size() == 1)
        activeRxLink = rxLinks.back();
    else
        activeRxLink = rxLinks[intrand(rxLinks.size())];

    EV_DETAIL << "Selected active: " << activeRxLink->str() << endl;
    return activeRxLink;
}

units::values::Hz Ieee802154eMac::getCurrentFrequency() {
    Enter_Method_Silent();
    return hopping->channelToCenterFrequency(currentChannel);
}

bool isControlPacket(std::string packetName) {
    std::list<std::string> ctrlPktNames = {"NSpacket", "NApacket", "DAO"};

    for (auto name : ctrlPktNames)
        if (packetName.find(name) != std::string::npos) {
            EV_DETAIL << packetName << " is a control packet" << endl;
            return true;
        }

    EV_DETAIL << packetName << " is NOT a control packet" << endl;
    return false;
}


int Ieee802154eMac::selectVirtualQueue(MacAddress nbrAddr) {
//    EV_DETAIL << "Selecting active virtual queue with " << nbrAddr << endl;

    // FIXME: set dynamically
    // If there're any packets in absolute priority queues (control and SP traffic), select them
    std::vector<int> absPrioLinkIds = {-2, -1};
    for (auto linkId: absPrioLinkIds)
        if (neighbor->getVirtualQueueSizeAt(nbrAddr, linkId) > 0)
            return linkId;

    // FIXME: set dynamically
    // If no absolute priority packets are there, take a look at other, WRR-operated queues
    std::vector<int> wrrLinkIds = {0, 1}; // the order of elements should be ascending! Higher means lower prio

    // If WRR scheduling is disabled, just the first non-empty queue with higher priority will be selected
    if (!par("wrrEnabled"))
    {
        for (auto linkId: wrrLinkIds)
            if (neighbor->getVirtualQueueSizeAt(nbrAddr, linkId) > 0)
                return linkId;
    }
    else
    {
        //TODO: make this generic for any number of virtual link IDs
        auto npQueueSize = neighbor->getVirtualQueueSizeAt(nbrAddr, wrrLinkIds[0]);
        auto beQueueSize = neighbor->getVirtualQueueSizeAt(nbrAddr, wrrLinkIds[1]);

        EV_DETAIL << "NP: " << npQueueSize << " packets" << endl;
        EV_DETAIL << "BE: " << beQueueSize << " packets" << endl;

        // Check if there're packets in more than one queue operated by WRR at all,
        // if not, there's no contention and we can just select non-empty queue
        if (npQueueSize == 0 && beQueueSize > 0) {
            EV_DETAIL << "NP queue empty, selected BE" << endl;
            return wrrLinkIds[1];
        }


        if (npQueueSize > 0 && beQueueSize == 0) {
            EV_DETAIL << "BE queue empty, selected NP" << endl;
            return wrrLinkIds[0];
        }


        // Now, if both queues have packets
        if (npQueueSize > 0 && beQueueSize > 0)
        {
            // Check whether NP has used all its turns
            if (wrr_np_ctn == w_np) {
                // Check if BE has also used all its turns,
                // if yes, reset the counters (one WRR round)
                if (wrr_be_ctn == w_be) {
                    wrr_np_ctn = 0;
                    wrr_be_ctn = 0;

                    EV_DETAIL << "WRR round reset" << endl;
                    // Since we reset the counters, a new round is started,
                    // NP has packets in queue and has the right to occupy the cell
                    wrr_np_ctn++;
                    return wrrLinkIds[0];
                }
                // BE has not used all its turns, increment counter and let BE traffic use the cell
                else {
                    wrr_be_ctn++;
                    EV_DETAIL << "Selected WRR BE queue, turns used: " << wrr_be_ctn << "/" << w_be << endl;
                    return wrrLinkIds[1];
                }
            }
            // NP has still some turns left and can occupy the cell
            else {
                wrr_np_ctn++;
                EV_DETAIL << "Selected WRR NP queue, turns used: " << wrr_np_ctn << "/" << w_np << endl;
                return wrrLinkIds[0];
            }
        }
    }

    EV_DETAIL << "No virtual queue with packets found" << endl;
    return 0;
}


void Ieee802154eMac::updateStatusIdle(t_mac_event event, cMessage *msg) {
    switch (event) {
    case EV_TIMER_SLOT: {
        EV_DETAIL << "(1) FSM State IDLE_1, EV_TIMER_SLOT: startTimerSlot -> idle." << endl;
        // directly schedule next slot
        startTimer(TIMER_SLOT);

        // start of new slot
        // Done in  startTimer(Timer_Slot);
        //currentAsn = asn.getAsn(simTime());
//        currentLink = schedule->getLinkFromASN(currentAsn);

        currentLink = selectActiveLink(schedule->getLinksFromASN(currentAsn));

        if (!currentLink)
            return;


        currentChannel = hopping->channel(currentAsn, currentLink->getChannelOffset());

        /**
         * Disable channel hopping for minimal cells
         * if corresponding flag is set, or, only for WAIC band, the channel offset is set to the highest value (39)
         *
         */
        if (sf && currentLink->getAddr() == MacAddress::BROADCAST_ADDRESS
                && (sf->par("minCellChannelOffset").intValue() == 39 || hopping->par("disableMinCellHopping").boolValue()))
        {
            currentChannel = hopping->getMinChannel() + sf->par("minCellChannelOffset").intValue();
        }


//        // Only for 6TiSCH:
//        // If the minimal cell channel offset is set to the maximum value, i.e. highest frequency,
//        // to ensure reliable connectivity, this cell should NOT channel-hop
//        // TODO: check radio center frequency and throw an error if channel number 39 is used in ISM!!!
//        if (sf && sf->par("minCellChannelOffset").intValue() == 39) // max WAIC channel
//        {
//            if (currentLink->getAddr() == MacAddress::BROADCAST_ADDRESS) // checking if current link is a minimal cell
//                currentChannel = sf->par("minCellChannelOffset").intValue();
//            else
//                currentChannel = hopping->channel(currentAsn, currentLink->getChannelOffset());
//        }
//        else
//            currentChannel = hopping->channel(currentAsn, currentLink->getChannelOffset());

        emit(currentFreqSignal, hopping->channelToCenterFrequencyPlain(currentChannel));

        auto freq = hopping->channelToCenterFrequency(currentChannel);

        EV_DETAIL << currentLink->str() << endl;

        // unconditionally emit a signal at slot start when we already have link infos
        emitSignal(NBSLOT);

        neighbor->reset();
        EV_DETAIL << "ASN #" << currentAsn << ", channel " << currentChannel
                << ", frequency " << freq << ", MAC: " << interfaceEntry->getMacAddress() << endl;

        if (currentLink->isTx())
        {
            int currentVirtualLinkID = selectVirtualQueue(currentLink->getAddr());
            auto queueSize = neighbor->getVirtualQueueSizeAt(currentLink->getAddr(), currentVirtualLinkID);
            neighbor->printQueue();

            if (queueSize > 0) {

                neighbor->setVirtualQueue(currentLink->getAddr(), currentVirtualLinkID);

                // Dedicated link
                if (!currentLink->isShared())
                {
                    neighbor->setDedicated(true);
                    updateMacState(CCA_3);
                    startTimer(TIMER_CCA);
                    configureRadio(freq, IRadio::RADIO_MODE_RECEIVER);
                }
                // Shared link
                else if (currentLink->isShared())
                {
                    // Backoff == 0
                    if (neighbor->getCurrentTschCSMA()->checkBackoff())
                    {
                        updateMacState(CCA_3);
                        startTimer(TIMER_CCA);
                        configureRadio(freq, IRadio::RADIO_MODE_RECEIVER);
                    }
                }
                // Nothing to send -> RX
                else if (currentLink->isRx())
                {
                    updateMacState(RECEIVEFRAME_6);
                    startTimer(TIMER_SLOTEND);
                    configureRadio(freq, IRadio::RADIO_MODE_RECEIVER);
                }
            }
            // Its a transmission link (and shared and with broadcast address) but the specific queue is empty,
            // else it would be impossible to reach this point
            else if (currentLink->isShared() && currentLink->getAddr().isBroadcast())
            {
                updateMacState(RECEIVEFRAME_6);
                startTimer(TIMER_SLOTEND);
                configureRadio(freq, IRadio::RADIO_MODE_RECEIVER);
            }
        }
        // Nothing to send -> RX
        else if (currentLink->isRx())
        {
            updateMacState(RECEIVEFRAME_6);
            startTimer(TIMER_SLOTEND);
            configureRadio(freq, IRadio::RADIO_MODE_RECEIVER);
        }

        // Decrement backoff window for all queues directed at link->getAddr
        if(currentLink->isTx() && currentLink->isShared())
            neighbor->updateAllBackoffWindows(currentLink->getAddr(),schedule);

        break;
    }
    default:
        fsmError(event, msg);
        break;
    }
}

void Ieee802154eMac::configureRadio(Hz centerFrequency /*= NAN*/,
        int mode /*= -1*/) {
    auto configureCommand = new ConfigureRadioCommand();
    auto request = new Message("changeChannel", RADIO_C_CONFIGURE);

    configureCommand->setCenterFrequency(centerFrequency);
    configureCommand->setRadioMode(mode);
    request->setControlInfo(configureCommand);

    sendDown(request);
}

void Ieee802154eMac::flushQueue(MacAddress neighborAddr, int vlinkId) {
    neighbor->flushQueue(neighborAddr, vlinkId); // TODO: access TschNeighbor directly
}

void Ieee802154eMac::flush6pQueue(MacAddress neighborAddr) {
    neighbor->flush6pQueue(neighborAddr); // TODO: access TschNeighbor directly
}

void Ieee802154eMac::attachSignal(Packet *mac, simtime_t_cref startTime) {
    simtime_t duration = mac->getBitLength() / bitrate;
    mac->setDuration(duration);
}

void Ieee802154eMac::updateStatusCCA(t_mac_event event, cMessage *msg) {
    switch (event) {
    case EV_TIMER_CCA: {
        EV_DETAIL << "(25) FSM State CCA_3, EV_TIMER_CCA" << endl;
        bool isIdle = true;
        if(useCCA){
            isIdle = radio->getReceptionState() == IRadio::RECEPTION_STATE_IDLE;
        }
        if (isIdle) {
            EV_DETAIL << "(3) FSM State CCA_3, EV_TIMER_CCA, [Channel Idle]: -> TRANSMITFRAME_4." << endl;
            updateMacState(TRANSMITFRAME_4);
            radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
            Packet *mac =  check_and_cast<Packet *>(neighbor->getCurrentNeighborQueueFirstPacket()->dup());
            attachSignal(mac, simTime() + macTsRxTx);
            // give time for the radio to be in Tx state before transmitting
            // TODO: Strangely the total amount for transmission time is 192us longer then the theoretical one, needs to be fixed !
            sendDelayed(mac, macTsRxTx, lowerLayerOutGateId);
            nbTxFrames++;

            emitSignal(NBTXFRAMES);

            EV << "Number of transmitted frames: " << nbTxFrames << endl;
        } else {
            // Channel was busy, increment 802.15.4 backoff timers as specified.
            EV_DETAIL << "(7) FSM State CCA_3, EV_TIMER_CCA, [Channel Busy]: " << " skipping slot." << endl;

            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            manageFailedTX(false);
            updateMacState(IDLE_1);
        }
        break;
    }
    default:
        fsmError(event, msg);
        break;
    }
}

void Ieee802154eMac::updateStatusTransmitFrame(t_mac_event event, cMessage *msg) {
    if (event != EV_FRAME_TRANSMITTED)
        fsmError(event, msg);

    Packet *packet = neighbor->getCurrentNeighborQueueFirstPacket();
    const auto& csmaHeader = packet->peekAtFront<Ieee802154eMacHeader>();

    bool expectAck = useMACAcks;
    if (!csmaHeader->getDestAddr().isBroadcast() && !csmaHeader->getDestAddr().isMulticast())
    {
        //unicast
        EV_DETAIL << "(4) FSM State TRANSMITFRAME_4, EV_FRAME_TRANSMITTED [Unicast]: ";
    } else {
        //broadcast
        EV_DETAIL << "(27) FSM State TRANSMITFRAME_4, EV_FRAME_TRANSMITTED [Broadcast] ";
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
}

void Ieee802154eMac::updateStatusWaitAck(t_mac_event event, cMessage *msg) {
    assert(useMACAcks);
    switch (event) {
    case EV_ACK_RECEIVED: {
        EV_DETAIL << "(5) FSM State WAITACK_5, EV_ACK_RECEIVED: ProcessAck..." << endl;
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        if (rxAckTimer->isScheduled())
            cancelEvent(rxAckTimer);
        cMessage *mac = neighbor->getCurrentNeighborQueueFirstPacket();
        neighbor->removeFirstPacketFromQueue();
        neighbor->terminateCurrentTschCSMA();
        neighbor->reset();
        emit(packetSentSignal, mac, nullptr);
        delete mac;
        delete msg;
        updateMacState(IDLE_1);
        break;
    }
    case EV_ACK_TIMEOUT:
        EV_DETAIL << "(12) FSM State WAITACK_5, EV_ACK_TIMEOUT:"
                         << " start TschCSMA,incrementCounter/dropPacket" << endl;
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

        if (!neighbor->isDedicated() && !neighbor->getCurrentTschCSMAStatus())
            neighbor->startTschCSMA();
        else
            manageFailedTX(true);

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

list<uint64_t> Ieee802154eMac::getNeighborsInRange() {
    list<uint64_t> resultingList;

    auto nodeId = this->interfaceEntry->getMacAddress().getInt(); // our own MAC
    auto radio = this->getRadio();
    auto medium = dynamic_cast<const physicallayer::RadioMedium*>(radio->getMedium());
    auto limitcache = dynamic_cast<const physicallayer::MediumLimitCache*>(medium->getMediumLimitCache());
    auto range = limitcache->getMaxCommunicationRange(radio).get(); // meaningless with realistic radio models
    auto myCoords = radio->getAntenna()->getMobility()->getCurrentPosition();
    // we extract the topology here and filter for nodes that have the property @6tisch set.
    // The property has to be set within the top-level ned-file (contains your network) e.g. like this:
    //
    // host[numHosts]: WirelessHost {
    //     parameters:
    //         @6tisch;
    // }
    //
    cTopology topo;
    topo.extractByProperty("6tisch");

    for (int i = 0; i < topo.getNumNodes(); i++) {
        auto host = topo.getNode(i)->getModule();
        auto mobilityModule = dynamic_cast<IMobility *>(host->getSubmodule("mobility"));
        auto interfaceModule = dynamic_cast<InterfaceTable *>(host->getSubmodule("interfaceTable"));
        auto coords = mobilityModule->getCurrentPosition();
        inet::MacAddress addr;

        // look for a WirelessInterface and get it's address
        // TODO currently takes first WirelessInterface, does not filter for interface type
        if (interfaceModule->getNumInterfaces() > 0)
            for (int y = 0; y < interfaceModule->getNumInterfaces(); y++) {
                if (strcmp(interfaceModule->getInterface(y)->getNedTypeName(), "inet.linklayer.common.WirelessInterface") == 0) {
                    addr = interfaceModule->getInterface(y)->getMacAddress();
                    break;
                }
            }

        // we found no mac address to use or we found ourself
        if (addr.isUnspecified() || addr.getInt() == nodeId) {
            EV << "No MAC address found?" << endl;
            continue;
        }

        // verify distance to ourself
        if (myCoords.distance(coords) <= range) {
            resultingList.push_back(addr.getInt());
            EV << "node " << addr.str() << " (" << coords.str() << ") is a neighbor of "
                    << MacAddress(nodeId).str() << " (" << myCoords.str() << ")" << endl;
        }
    }

    return resultingList;
}

void Ieee802154eMac::manageFailedTX(bool recordStats) {
    neighbor->failedTX();

    auto rtxAttempts = neighbor->getCurrentTschCSMA()->getNB();

    EV_DETAIL << "RTX attempts: " << rtxAttempts << endl;

    if (rtxAttempts > (int) macMaxFrameRetries) {
        EV_DETAIL << "Packet was re-transmitted > " << macMaxFrameRetries
                << " times and an ACK was never received. The packet is dropped." << endl;
        cMessage *mac = neighbor->getCurrentNeighborQueueFirstPacket();
        neighbor->removeFirstPacketFromQueue();
        neighbor->terminateCurrentTschCSMA();
        PacketDropDetails details;
        details.setReason(RETRY_LIMIT_REACHED);
        details.setLimit(macMaxFrameRetries);
        emit(packetDroppedSignal, mac, &details);
        emit(linkBrokenSignal, mac);
        delete mac;
    } else {
        auto pkt = neighbor->getCurrentNeighborQueueFirstPacket();
        if (isSmokeAlarmPacket(pkt) && recordStats)
        {
            emit(pktRetransmittedSignal, 1);

            // If the cell is shared, it's likely the downlink one.
            if (!neighbor->isDedicated())
                emit(pktRetransmittedDownlinkSignal, 1);

            if (lastAppPktArrivalTimestamp > 0)
                emit(pktInterarrivalTimeSignal, simTime().dbl() - lastAppPktArrivalTimestamp);

            lastAppPktArrivalTimestamp = simTime().dbl();
        }

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
        //        sendDelayed(ackMessage, macTsRxTx, lowerLayerOut);
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

        EV_DEBUG << "(startTimer) slotTimer value=" << at << endl;
        scheduleAt(simTime() + at, slotTimer);
    } else if (timer == TIMER_CCA) {
        simtime_t ccaTime = rxSetupTime + ccaDetectionTime;
        EV_DEBUG << "(startTimer) ccaTimer value=" << ccaTime
                         << "(rxSetupTime,ccaDetectionTime:" << rxSetupTime
                         << "," << ccaDetectionTime << ")." << endl;
        scheduleAt(simTime() + rxSetupTime + ccaDetectionTime, ccaTimer);
    } else if (timer == TIMER_SIFS) {
        assert(useMACAcks);
        EV_DEBUG << "(startTimer) sifsTimer value=" << macTsTxAckDelay << endl;
        scheduleAt(simTime() + macTsTxAckDelay, sifsTimer);
    } else if (timer == TIMER_RX_ACK) {
        assert(useMACAcks);
//        EV_DETAIL << "(startTimer) rxAckTimer value=" << macAckWaitDuration
//                                 << endl;
        EV_DEBUG << "(startTimer) rxAckTimer value="
                <<  macTsRxAckDelay + macTsAckWait + macTsMaxAck << endl;
        //scheduleAt(simTime() + macAckWaitDuration, rxAckTimer);
        scheduleAt(simTime() + macTsRxAckDelay + macTsAckWait + macTsMaxAck, rxAckTimer);
    } else if (timer == TIMER_HOPPING) {
        //assert(useMACAcks); TODO verify if channel hopping is enabled?
        // or always on and rely on TschHopping class to disable?
        EV_DEBUG << "(startTimer) hoppingTimer value=" << channelSwitchingTime
                         << endl;
        scheduleAt(simTime() + channelSwitchingTime, hoppingTimer);
    } else if (timer == TIMER_SLOTEND) {
        // TODO currently scheduled right before slot end,
        // but could be done more strict to increase radio sleep
        // TODO This value should not be calculated this way,
        // but due to the fact that mactsCcaOffset and macTsRxOffset are not considered
        // it should be around 0.48
        EV_DEBUG << "(startTimer) slotendTimer value="
                         << (macTsTimeslotLength * 0.48) << endl;
        scheduleAt(simTime() + (macTsTimeslotLength * 0.48), slotendTimer);
    } else {
        EV << "Unknown timer requested to start:" << timer << endl;
    }
}

/*
 * Binds timers to events and executes FSM.
 */
void Ieee802154eMac::handleSelfMessage(cMessage *msg) {
    // TEST message kind for simulating lossy links
    if (msg->getKind() == MAC_ENABLE_DROPS) {
        pLinkCollision = par("pLinkCollision").doubleValue();
        delete msg;
        return;
    }

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

bool Ieee802154eMac::artificiallyDropAppPacket(Packet *packet) {
    std::string packetName(packet->getFullName());

    if (packetName.find(std::string("Udp")) != std::string::npos) {

        auto r = uniform(0, 1, 2);
        bool drop = r < pLinkCollision;

//        EV_DETAIL << "r = " << r;
//
//        if (drop)
//            EV_DETAIL << ", dropping Udp packet #" << udpDroppedCtn++ << endl;
//        else
//            EV_DETAIL << ", forwarding Udp packet #" << udpSentCtn++ << endl;

        return drop;
    }

    return false;
}

int Ieee802154eMac::getMacMaxBackoff() {
    return pow(2, neighbor->getMacMaxBe()) * macMaxFrameRetries;
}

void Ieee802154eMac::recordIncorrectlyReceived(Packet *packet) {
    const auto& csmaHeader = packet->peekAtFront<Ieee802154eMacHeader>();
    const MacAddress& src = csmaHeader->getSrcAddr();
    const MacAddress& dest = csmaHeader->getDestAddr();

    auto macAddrPair = new MacHeaderAddresses(src, dest);
    auto storedEntry = packetsIncorrectlyReceived.find( *macAddrPair );

    if (storedEntry == packetsIncorrectlyReceived.end())
        packetsIncorrectlyReceived.insert( pair<MacHeaderAddresses, int>(*macAddrPair, 1) );
    else
        storedEntry->second++;
}


bool Ieee802154eMac::drop6pPacket(Packet *packet, std::string cmdType, std::string pktType) {
    if (simTime() < 50 || simTime() > 100)
        return false;

    std::string pktName(packet->getFullName());

    if (pktName.find(cmdType) == std::string::npos || pktName.find(pktType) == std::string::npos)
        return false;

    EV_DETAIL << "Artificially dropping 6P " << cmdType << " " << pktType << endl;

    return true;
}

/**
 * Compares the address of this Host with the destination address in
 * frame. Generates the corresponding event.
 */
void Ieee802154eMac::handleLowerPacket(Packet *packet) {
    // Either packet has a bit error, or an *artificial* link collision probability applies
    if ( (packet->hasBitError() && !ignoreBitErrors)
            || artificiallyDropAppPacket(packet)) // || drop6pPacket(packet, "TSCH", "Ack")
    {
        EV << "Received " << packet << " contains bit errors or collision, dropping it\n";
        PacketDropDetails details;
        details.setReason(INCORRECTLY_RECEIVED);
        recordIncorrectlyReceived(packet);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
        return;
    }
    if (ignoreBitErrors)
        packet->setBitError(false);
    const auto& csmaHeader = packet->peekAtFront<Ieee802154eMacHeader>();
    const MacAddress& src = csmaHeader->getSrcAddr();
    const MacAddress& dest = csmaHeader->getDestAddr();
    long ExpectedNr = 0;
    MacAddress address = interfaceEntry->getMacAddress();
    EV_DETAIL << "Received frame name= " << csmaHeader->getName()
                     << ", myState=" << macState << " src=" << src << " dst="
                     << dest << " myAddr=" << address << endl;
    if (dest == address) {
        emit(pktRecFromLowerSignal, (long) (src.getInt())); // notify SF about to keep track of the neighbor

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
                Packet *firstPacket = static_cast<Packet *>(neighbor->getCurrentNeighborQueueFirstPacket());
                const auto& csmaHeader = firstPacket->peekAtFront<Ieee802154eMacHeader>();
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

InterfaceEntry* Ieee802154eMac::getInterfaceEntry(){
    return this->interfaceEntry;
}


}
