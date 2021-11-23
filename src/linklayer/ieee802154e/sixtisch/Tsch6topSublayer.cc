/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 * Implementation of the 6top sublayer as a MiXiM BaseApplLayer.
 *
 * Note that the 6top sublayer isn't *actually* located on layer 7, but this is
 * the only interface that lets you not have a gate to any upper layer. And since
 * the 6top sublayer serves as kind of a mini application layer to the MAC layer,
 * (at least for now, in case it communicates with the routing protocol for
 * optimization purposes this probably has to change) this is what it is now.
 *
 * For more details on the sublayer see
 * https://tools.ietf.org/html/draft-ietf-6tisch-architecture
 *
 * Copyright (C) 2021  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2021  Yevhenii Shudrenko
 *           (C) 2017  Lotte Steenbrink
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

#include "Tsch6topSublayer.h"

//#include "NetworkTopologyMockManager.h"
#include <memory>
//#include "TschMacWaic.h"

#include "../TschLink.h"
#include "TschSF.h"
//#include "msg/tschSpectrumSensingResult_m.h"
#include "tsch6pPiggybackTimeoutMsg_m.h"
#include "WaicCellComponents.h"
#include "inet/physicallayer/common/packetlevel/RadioMedium.h"
#include "inet/physicallayer/common/packetlevel/MediumLimitCache.h"
#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/common/InterfaceTable.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/common/ProtocolTag_m.h"
#include "SixpDataChunk_m.h"
#include "SixpHeaderChunk_m.h"
#include "../../../common/VirtualLinkTag_m.h"
#include "inet/common/packet/chunk/BytesChunk.h"
#include "../Ieee802154eMacHeader_m.h"
#include "inet/common/ProtocolGroup.h"
//#include "TschImprovisedTests.h"

using namespace omnetpp;
using namespace tsch;
using namespace inet;

Define_Module(Tsch6topSublayer);

Tsch6topSublayer::Tsch6topSublayer() {
}

Tsch6topSublayer::~Tsch6topSublayer() {
    for (auto& entry: piggybackableData) {
        for (auto & msg: entry.second) {
            delete msg;
        }
    }
    for (auto& entry: pendingPatternUpdates) {
        delete entry.second;
    }
}

void Tsch6topSublayer::initialize(int stage) {
    if (stage == 0) {
        EV_DETAIL << "Initializing Tsch6topSublayer" << endl;

        PIGGYBACKING_BACKOFF = SimTime((int) par("piggybackingBackoff"), SIMTIME_MS);

        /* initialize gates to submodules */
        linkInfoControlIn = findGate("linkInfoControlIn");

        /* initialize gates to MAC layer */
        lowerLayerOut = findGate("lowerLayerOut");
        lowerLayerIn = findGate("lowerLayerIn");

        //lowerControlOut = findGate("lowerControlOut");
        lowerControlIn = findGate("lowerControlIn");
        pSFStarttime = par("sfstarttime");

        sent6pClearSignal = registerSignal("sent6pClear");
        sent6pAddSignal = registerSignal("sent6pAdd");
        sent6pDeleteSignal = registerSignal("sent6pDelete");
        sent6pRelocateSignal = registerSignal("sent6pRelocate");
        sent6pResponseSignal = registerSignal("sent6pResponse");

        numConcurrentTransactionErrors = 0;
        numTimeouts = 0;

        // TODO: just set to sim-time-limit?

        int ttl = getParentModule()->getSubmodule("sf")->par("timeout");
        TxQueueTTL = SimTime(ttl, SIMTIME_MS);

    } else if (stage == 4) {
        auto module = getParentModule()->getParentModule();
        Ieee802154eMac* mac = dynamic_cast<Ieee802154eMac *>(module->getSubmodule("mac", 0));

        mac->subscribe(inet::packetDroppedSignal, this);
        mac->subscribe(inet::packetSentSignal, this);

        schedule = dynamic_cast<TschSlotframe*>(module->getSubmodule("schedule", 0));
        pTschLinkInfo = (TschLinkInfo*) getParentModule()->getSubmodule("linkinfo");
        if (!mac) {
            EV_ERROR << "Tsch6topSublayer: no mac submodule found" << endl;
            return;
        }
        pNodeId = mac->getInterfaceEntry()->getMacAddress().getInt();
        EV_DETAIL << " for node " << pNodeId << endl;
        std::list<uint64_t> neighbors = getNeighborsInRange(pNodeId, mac);

        std::list<uint64_t>::iterator it;

        pTschSF = (TschSF*) getParentModule()->getSubmodule("sf");
        pSFID = pTschSF->getSFID();

        /* only start SF once tschmacwaic is initialized and we can actually
           figure out who our neighbors are */
        cMessage* msg = new cMessage();
        msg->setKind(SF_START);

        scheduleAt(simTime()+pSFStarttime, msg);
    }
};

void Tsch6topSublayer::finish() {
    recordScalar("numTimeouts", numTimeouts);
    recordScalar("numConcurrentTransactionErrors", numConcurrentTransactionErrors);
}

Packet* Tsch6topSublayer::handleExternalMessage(cMessage* msg) {
    Packet* response = NULL;
    auto arrivalGateId = msg->getArrivalGateId();

    if (arrivalGateId == linkInfoControlIn) {
        tschLinkInfoTimeoutMsg* tom = dynamic_cast<tschLinkInfoTimeoutMsg*> (msg);
        if (tom)
            response = handleTransactionTimeout(tom);
    }
    else if (arrivalGateId == lowerLayerIn) {
        auto pkt = dynamic_cast<Packet*> (msg);
        if (pkt)
            response = handle6PMsg(pkt);
    }
    else EV_WARN << "Received message through unexpected gate - " << arrivalGateId << endl;

    delete msg;
    return response;
}

Packet* Tsch6topSublayer::handleSelfMessage(cMessage* msg)
{
    switch (msg->getKind()) {
        case SF_START: {
            EV_DETAIL << "Starting Scheduling Function at node " << MacAddress(pNodeId) << endl;
            pTschSF->start();
            break;
        }
        case PIGGYBACK_TIMEOUT: {
            tsch6pPiggybackTimeoutMsg* pgb = dynamic_cast<tsch6pPiggybackTimeoutMsg*> (msg);

            if (!pgb || !pgb->getInTransit())
                break;

            /* a piggybacking timeout has expired, send data stand-alone*/
            // TODO: let blacklist updates "accumulate" until they are sent off! otherwise a huge
            // TODO: is the SIGNAL a transaction too? if so, call prepLinkForRequest!
            pgb->setInTransit(true);
            uint64_t destId = pgb->getDestId();
            return createSignalRequest(destId, pTschLinkInfo->getSeqNum(destId), pgb->getContextPointer(), pgb->getPayloadSz());
        }
        default: EV_WARN << "Unknown self-message received - " << msg->getKind() << endl;
    }

    delete msg;
    return NULL;
}

void Tsch6topSublayer::handleMessage(cMessage* msg) {
    Packet* response = NULL;
    tsch6topCtrlMsg* ctrl = NULL;

    response = msg->isSelfMessage() ? handleSelfMessage(msg) : handleExternalMessage(msg);

    if (response != NULL) {
        /* a response was created somewhere in the handling process, send it */
        sendMessageToRadio(response);

        emit(sent6pResponseSignal, 1);
    }
}

void Tsch6topSublayer::sendMessageToRadio(cMessage *msg, double delay) {
    if (!msg) {
        EV_WARN << "sendMessageToRadio: msg pointer is null!" << endl;
        return;
    }

    if (delay > 0)
        sendDelayed(msg, delay, lowerLayerOut);
    else
        send(msg, lowerLayerOut);
}
/**void Tsch6topSublayer::sendControlDown(cMessage *msg) {
    if (msg) {
        send(msg, lowerControlOut);
    } else {
        EV_WARN <<"sendControlDown: msg pointer is null!" << endl;
    }
}*/

bool Tsch6topSublayer::sendAddRequest(uint64_t destId, uint8_t cellOptions,
                            int numCells, std::vector<cellLocation_t> &cellList, int timeout)
{
    return sendAddRequest(destId, cellOptions, numCells, cellList, timeout, 0);
}

bool Tsch6topSublayer::sendAddRequest(uint64_t destId, uint8_t cellOptions,
                            int numCells, std::vector<cellLocation_t> &cellList,
                            int timeout, double delay)
{
    Enter_Method_Silent();

    if (pTschLinkInfo->inTransaction(destId)) {
        EV_ERROR <<"Can't send ADD request during open transaction" << endl;
        return false;
    }

    /* Calculate simtime at which this transaction will time out. since simtime_t is
       always counted in (fractions of) seconds, we need to convert timeout first */
    simtime_t absoluteTimeout = getAbsoluteTimeout(timeout);
    EV_DETAIL << "Sending ADD to " << MacAddress(destId) << ", timeout scheduled at: "
            << absoluteTimeout << endl;
    uint8_t seqNum = prepLinkForRequest(destId, absoluteTimeout);

    auto pkt = createAddRequest(destId, seqNum, cellOptions, numCells, cellList, absoluteTimeout);

    pTschLinkInfo->setLastKnownCommand(destId, CMD_ADD);
    pTschLinkInfo->setLastLinkOption(destId, (uint8_t) cellOptions);

    /* record statistics */
    emit(sent6pAddSignal, 1);

    sendMessageToRadio(pkt, delay);

    return true;
}

void Tsch6topSublayer::sendDeleteRequest(uint64_t destId, uint8_t cellOptions, int numCells,
        std::vector<cellLocation_t> cellList, int timeout)
{
    Enter_Method_Silent();

    if (pTschLinkInfo->inTransaction(destId)) {
        EV_ERROR <<"Can't send DELETE request during open transaction" << endl;
        return;
    }

    if (!cellList.size()) {
        EV_WARN << "Received DELETE req with empty cellList" << endl;
        return;
    }

    /* calculate simtime at which this transaction will time out. since simtime_t is
       always counted in (fractions of) seconds, we need to convert timeout first */
    simtime_t absoluteTimeout = getAbsoluteTimeout(timeout);
    uint8_t seqNum = prepLinkForRequest(destId, absoluteTimeout);

    auto pkt = createDeleteRequest(destId, seqNum, cellOptions, numCells, cellList, absoluteTimeout);

    pTschLinkInfo->setLastKnownCommand(destId, CMD_DELETE);
    pTschLinkInfo->setLastLinkOption(destId, (uint8_t) cellOptions);

    /* record statistics */
    emit(sent6pDeleteSignal, 1);

    sendMessageToRadio(pkt);
}

void Tsch6topSublayer::sendRelocationRequest(uint64_t destId, uint8_t cellOptions, int numCells,
                            std::vector<cellLocation_t> &relocationCellList,
                            std::vector<cellLocation_t> &candidateCellList,
                            int timeout)
{
    Enter_Method_Silent();

    if (pTschLinkInfo->inTransaction(destId)) {
        EV_ERROR << "Can't send RELOCATE request during open transaction" << endl;
        return;
    }

    simtime_t absoluteTimeout = getAbsoluteTimeout(timeout);
    pTschLinkInfo->setRelocationCells(destId, relocationCellList, getCellOption(cellOptions));
    uint8_t seqNum = prepLinkForRequest(destId, absoluteTimeout);

    auto pkt = createRelocationRequest(destId, seqNum, cellOptions, numCells,
            relocationCellList, candidateCellList, absoluteTimeout);

    pTschLinkInfo->setLastKnownCommand(destId, CMD_RELOCATE);
    pTschLinkInfo->setLastLinkOption(destId, (uint8_t) cellOptions);

    /* record statistics */
    emit(sent6pRelocateSignal, 1);

    sendMessageToRadio(pkt);
}

void Tsch6topSublayer::sendClearRequest(uint64_t destId, int timeout) {
    Enter_Method_Silent();

    if (!destId)
        throw cRuntimeError("Trying to send CLEAR to node 00-00-00-00-00-00");

    if (!pTschLinkInfo->linkInfoExists(destId))
        throw cRuntimeError("Attempt to send a CLEAR when no link info available");

    uint8_t seqNum = pTschLinkInfo->getLastKnownSeqNum(destId);
    simtime_t absoluteTimeout = getAbsoluteTimeout(timeout);

    /* CLEAR requests end a transaction, so we can override the current status
       no matter what. However seqnum & cells are only reset after a RC_SUCCESS
       response is received, so we still need to note that we sent a CLEAR */
    if (!pTschLinkInfo->inTransaction(destId))
        pTschLinkInfo->setInTransaction(destId, absoluteTimeout);

    auto pkt = createClearRequest(destId, seqNum);

    pTschLinkInfo->setLastKnownType(destId, MSG_REQUEST);
    pTschLinkInfo->setLastKnownCommand(destId, CMD_CLEAR);

    /* record statistics */
    emit(sent6pClearSignal, 1);

    sendMessageToRadio(pkt);
}

void Tsch6topSublayer::piggybackData(uint64_t destId, void* payload, int payloadSz,
                                     int timeout) {
    Enter_Method_Silent();

    /* configure piggyback timeout (after this expires, data MUST be sent as Signal) */
    tsch6pPiggybackTimeoutMsg* msg = new tsch6pPiggybackTimeoutMsg();
    msg->setKind(PIGGYBACK_TIMEOUT);
    msg->setDestId(destId);
    msg->setPayloadSz(payloadSz);
    msg->setContextPointer(payload);
    msg->setInTransit(false);

    if (piggybackableData.find(destId) == piggybackableData.end()) {
        std::vector<tsch6pPiggybackTimeoutMsg*> v;
        piggybackableData[destId] = v;
    }

    /* save piggybackable data so that we can forward it and disable the timer
       if we manage to get the data out before the timeout expires */
    piggybackableData[destId].push_back(msg);

    /* start piggyback timeout */
    scheduleAt(getAbsoluteTimeout(timeout), msg);
}

Packet* Tsch6topSublayer::handle6PMsg(Packet* pkt) {
    Packet* response = NULL;

    auto hdr = pkt->popAtFront<tsch::sixtisch::SixpHeader>();
    auto data = pkt->popAtFront<tsch::sixtisch::SixpData>();
    auto addresses = pkt->getTag<MacAddressInd>();

    // TODO: lock linkinfo while handling it?!
    if (hdr->getSfid() != pSFID) {
        /* packet uses scheduling function different from ours */
        EV_ERROR << "Received 6P message sent by the wrong SF" << endl;
        uint64_t sender = addresses->getSrcAddress().getInt();
        uint8_t seqNum = hdr->getSeqNum();

        return createErrorResponse(sender, seqNum, RC_SFID, data->getTimeout());
    }

    /* packet is for us and valid, handle it */
    tsch6pMsg_t msgType = (tsch6pMsg_t) hdr->getType();

    switch (msgType) {
        case MSG_REQUEST: {
            response = handleRequestMsg(pkt, hdr, data);
            break;
        }
        case MSG_RESPONSE: {
            response = handleResponseMsg(pkt, hdr, data);
            break;
        }
        default: EV_WARN << "Unknown 6P message type" << endl;
    }

    return response;
}


Packet* Tsch6topSublayer::handleRequestMsg(Packet* pkt,
        inet::IntrusivePtr<const tsch::sixtisch::SixpHeader>& hdr,
        inet::IntrusivePtr<const tsch::sixtisch::SixpData>& data)
{
    Packet* response = NULL;

    auto addresses = pkt->getTag<MacAddressInd>();

    uint8_t seqNum = hdr->getSeqNum();
    uint64_t sender = addresses->getSrcAddress().getInt();
    tsch6pCmd_t cmd = (tsch6pCmd_t) hdr->getCode();
    std::vector<cellLocation_t> emptyCellList = {};

    EV_DETAIL << "Received MSG_REQUEST " << cmd << " from " << MacAddress(sender) << ", seqNum - " << +seqNum << endl;

    if (cmd == CMD_CLEAR) {
        /* We don't need to perform any of the (seqNum) checks. A CLEAR is
           always valid. */
        pTschLinkInfo->setLastKnownCommand(sender, cmd);
        pTschSF->handleResponse(sender, RC_SUCCESS, 0, {}); // Note: SF handles schedule cleanup, not 6P!
        /* "The Response Code to a 6P CLEAR command SHOULD be RC_SUCCESS unless
           the operation cannot be executed.  When the CLEAR operation cannot be
           executed, the Response Code MUST be set to RC_RESET." */
        return createClearResponse(sender, seqNum, RC_SUCCESS, data->getTimeout());
    }

    if (data->getTimeout() < simTime()) {
        /* received msg whose timeout already expired; ignore it. */
        /* QUICK FIX (we probably shouldn't start the timeout timer before
         * pkt transmsmission but after pkt received?)*/
        EV_DETAIL << "EXPIRED MSG " << endl;
        pTschLinkInfo->setLastKnownType(sender, MSG_REQUEST);

        return response;
    }

    /* TODO untangle the mess of checks following this, this was a quick fix */
    bool isDuplicate = seqNum != 0 && seqNum == pTschLinkInfo->getLastKnownSeqNum(sender)
            && cmd == pTschLinkInfo->getLastKnownCommand(sender)
            && MSG_REQUEST == pTschLinkInfo->getLastKnownType(sender)
            && pTschLinkInfo->getLastLinkOption(sender) == data->getCellOptions();

    if (pTschLinkInfo->inTransaction(sender) && cmd != CMD_SIGNAL && !isDuplicate) {
        /* as per the 6P standard, concurrent transactions are not allowed.
         * (but make sure you don't abort an ongoing transaction because initial request received twice) */
        EV_WARN <<"received new MSG_REQUEST during active transaction: ignoring request, sending RC_RESET with seqNum "
                << +seqNum << endl;
        return createErrorResponse(sender, seqNum, RC_RESET, data->getTimeout());
    }

    if (seqNum != 0 && seqNum != (pTschLinkInfo->getLastKnownSeqNum(sender))) {
        /* schedule inconsistency detected (sequence number!) */
        EV_WARN << "Received seqNum - " << unsigned(seqNum) << ", expected - "
                << unsigned(pTschLinkInfo->getLastKnownSeqNum(sender)) << endl;

        // The Draft mandates that we should send a RC_SEQNUM response but handling
        // those is a bit underspecified for 2-2way transactions so I'm going
        // to do it this way for now.
        pTschSF->handleInconsistency(sender, seqNum);
        return response;
    }

    if (cmd == CMD_SIGNAL) {
        /* SIGNALs currently don't behave like fully-fledged requests so
           we handle them separately*/
        auto pl = pkt->popAtBack<BytesChunk>(pkt->getDataLength(), Chunk::PF_ALLOW_NULLPTR);
        if (pl != nullptr && pTschLinkInfo->linkInfoExists(sender))
            /* there's piggybacked data and a link that's relevant to it, let
               the SF that uses this data handle it */
            pTschSF->handlePiggybackedData(sender, (void*) &(pl->getBytes()));

        return response;
    }

    if (getCellOptions_isMultiple(data->getCellOptions())) {
        /* multiple cell options have been set. The draft doesn't specify how to
         handle this yet. :(*/
        // TODO. still handle piggybacked blacklist update?
        EV_ERROR << "Multiple CellOptions set!" << endl;
        return response;
    }

    if (pTschLinkInfo->inTransaction(sender)) {
        EV_WARN << "Received new MSG_REQUEST during active transaction: "
                << "ignoring request, sending RC_RESET (noticed late)" << endl;

        return createErrorResponse(sender, seqNum, RC_RESET, data->getTimeout());
    }

    pTschLinkInfo->setLastKnownCommand(sender, cmd);

    simtime_t timeout = data->getTimeout();
    int numCells = data->getNumCells();
    // Type of link
    uint8_t cellOptions = (uint8_t) data->getCellOptions();

    auto pl = pkt->popAtBack<BytesChunk>(pkt->getDataLength(),  Chunk::PF_ALLOW_NULLPTR);

    if (pl != nullptr) {
        /* there's piggybacked data, let the SF that uses this data handle it */
        EV_DETAIL << "Node " << pNodeId << " handling piggybacked data" << std::endl;
        pTschSF->handlePiggybackedData(sender, (void*) &pl->getBytes());
    }

    if (!pTschLinkInfo->linkInfoExists(sender)) {
        /* We don't know this node yet, add link info for it */
        pTschLinkInfo->addLink(sender, true, timeout, seqNum);
        pTschLinkInfo->setLastKnownCommand(sender, cmd);
    }
    else if (seqNum == 0) // TODO: Check if this clause makes sense at all!
    {
        /* EXPERIMENTAL: first clear the schedule, only then the TschLinkInfo! */
        auto clearMsg = new tsch6topCtrlMsg();
        clearMsg->setDestId(sender);
        clearMsg->setDeleteCells(pTschLinkInfo->getCellLocations(sender)); // delete all scheduled cells except for auto
        updateSchedule(*clearMsg);

        /* node has been reset, clear all existing link information. */
        pTschLinkInfo->resetLink(sender, MSG_REQUEST);
    }

    pTschLinkInfo->setLastKnownType(sender, MSG_REQUEST);
    pTschLinkInfo->setInTransaction(sender, timeout);

    // TODO: Refactor this further/split into separate handler functions
    switch (cmd) {
        case CMD_ADD: {
            std::vector<cellLocation_t> cellList = data->getCellList();
            // TODO: do I need to copy this to make sure it doesn't go out of
            // scope when pkt is deleted?
            /* Since Tx cells for the sender are Rx cells for me and vice versa,
               results of getCellOptions_isRx/Tx() are inverted */
            int pickResult = pTschSF->pickCells(sender, cellList, numCells,
                                                !getCellOptions_isRX(cellOptions),
                                                !getCellOptions_isTX(cellOptions),
                                                getCellOptions_isSHARED(cellOptions));

            switch (pickResult) {
                case -EINVAL: {
                    EV_ERROR <<"Invalid input during SF cell pick" << endl;
                    break;
                }
                case -ENOSPC: {
                    /* no suitable cell(s) available. Send an (empty) success response
                       anyway, as mandated by the standard */
                    EV_WARN <<"No suitable cell(s) found" << endl;
                    std::vector<cellLocation_t> empty = {};
                    response = createSuccessResponse(sender, seqNum, empty, data->getTimeout());
                    break;
                }
                case 0:
                case -EFBIG: {
                    /* Enough cells found */
                    response = createSuccessResponse(sender, seqNum, cellList, data->getTimeout());

                    /* TODO: check if we need to split cellList into multiple messages!
                       -> either because cellList is too long or because we have RX, TX,
                       Shared cells in there (need 1 list per type) */
                    uint8_t myOpt = invertCellOption(getCellOption(cellOptions));
                    /* store potential change to our hopping sequence (will be activated
                       if LL ACK arrives within this timeslot) */
                    pendingPatternUpdates[sender] = setCtrlMsg_PatternUpdate(
                                                        pendingPatternUpdates[sender],
                                                        sender, myOpt, cellList, emptyCellList,
                                                        data->getTimeout());
                    break;
                }
                default: EV_ERROR << "Undefined result code of SF cell-picking: " << pickResult << endl;
            }
            break;
        }
        case CMD_DELETE: {
            std::vector<cellLocation_t> cellList = data->getCellList();
            EV_DETAIL << "DELETE cells: " << cellList << endl;

            if (cellList.size() < numCells) {
                EV_ERROR << "Cell list size is smaller than number of cells requested" << endl;
                /** Send the error response due to error in cell list */
                response = createErrorResponse(sender, seqNum, RC_CELLLIST, data->getTimeout());
            }
            else {
                // in response to delete request send RC_SUCCESS with empty cell list,
                // TODO: figure out how and when node sending DELETE request clears ITS local schedule
                std::vector<cellLocation_t> emptyList = {};
                response = createSuccessResponse(sender, seqNum, cellList, data->getTimeout());

                /* store potential change to our hopping sequence
                 * (activated if LL ACK arrives within this timeslot) */
                pendingPatternUpdates[sender] = setCtrlMsg_PatternUpdate(
                                                    pendingPatternUpdates[sender],
                                                    sender, MAC_LINKOPTIONS_RX, emptyList,
                                                    cellList, data->getTimeout());
            }
            break;
        }
        case CMD_RELOCATE: {
            std::vector<cellLocation_t> candidateCellList = data->getCellList();
            std::vector<cellLocation_t> relocCellList = data->getRelocationCellList();

            EV_DETAIL << "RELOCATE " << numCells << " cell(s): " << relocCellList
                    << "\nto (any of the following): " << candidateCellList << endl;

            /* all cells currently scheduled on this link */
            cellVector sharedCells = pTschLinkInfo->getCells(sender);
            uint8_t linkOption = invertCellOption(getCellOption(cellOptions));

            if (numCells == (int) relocCellList.size() && pTschLinkInfo->cellsInSchedule(sender, relocCellList, linkOption))
            {
                /* relocCellList has the right size and all its cells are actually
                   part of the link it is trying to delete them from; handle pkt. */

                /* Since Tx cells for the sender are Rx cells for me and vice versa,
                   results of getCellOptions_isRx/Tx() are inverted */
                int pickResult = pTschSF->pickCells(sender, candidateCellList, numCells,
                                            !getCellOptions_isRX(cellOptions),
                                            !getCellOptions_isTX(cellOptions),
                                            getCellOptions_isSHARED(cellOptions));

                uint8_t numRelocCells = candidateCellList.size();

                if ((pickResult == 0 || pickResult == -EFBIG) && numRelocCells > 0) {
                    /* delete first n cells from relocCellList from our link */
                    relocCellList.resize(numRelocCells);

                    uint8_t myOpt = invertCellOption(getCellOption(cellOptions));

                    /* store potential change to our hopping sequence (will be activated
                       if LL ACK arrives within this timeslot) */
                    pendingPatternUpdates[sender] = setCtrlMsg_PatternUpdate(
                                                        pendingPatternUpdates[sender],
                                                        sender, myOpt, candidateCellList,
                                                        relocCellList, data->getTimeout());
                }

                response = createSuccessResponse(sender, seqNum, candidateCellList, data->getTimeout());
            } else {
                /* that was an invalid RELOCATE message, respond accordingly */
                EV_DETAIL << "Received invalid RELOCATE request, sending error response" << endl;
                response = createErrorResponse(sender, seqNum, RC_CELLLIST, data->getTimeout());
            }
            break;
        }
        default: EV_WARN << "Unknown 6P command received - " << cmd << endl;
    }

    return response;
}


// TODO: refactor this to be more modular
Packet* Tsch6topSublayer::handleResponseMsg(Packet* pkt, inet::IntrusivePtr<const tsch::sixtisch::SixpHeader>& hdr,
        inet::IntrusivePtr<const tsch::sixtisch::SixpData>& data)
{
    Packet* response = NULL;

    if (!pkt || !hdr || !data)
        return response;

    auto addresses = pkt->getTag<MacAddressInd>();

    std::vector<cellLocation_t> emptyCellList = {};
    uint8_t seqNum = hdr->getSeqNum();
    uint64_t sender = addresses->getSrcAddress().getInt();
    tsch6pReturn_t returnCode = (tsch6pReturn_t) hdr->getCode();

    EV_DETAIL << "Node " << MacAddress(pNodeId) << " received MSG_RESPONSE from "
            << MacAddress(sender) << " with seqNum " << +seqNum << ", return code - " << returnCode << endl;
    if (data->getTimeout() < simTime()) {
        /* received msg whose timeout already expired; ignore it. */
        /* QUICK FIX (we probably shouldn't start the timeout timer before
         * pkt transmsmission but after pkt received?)*/
        EV_DETAIL << "EXPIRED MSG " << endl;
        return response;
    }

    if (!pTschLinkInfo->inTransaction(sender)) {
        /* There's no transaction this response corresponds to, ignore it */
        EV_WARN << "Received unexpected MSG_RESPONSE: not in transaction" << endl;
        return response;
    }

    if (pTschLinkInfo->getLastKnownType(sender) == MSG_RESPONSE) {
        EV_WARN << "Received unexpected MSG_RESPONSE: this node just sent a MSG_RESPONSE itself" << endl;
        return response;
    }

    if (returnCode == RC_RESET) {
        pTschLinkInfo->revertLink(sender, MSG_RESPONSE);
        EV_DETAIL << "Reverted link with " << MacAddress(sender) << endl;
        pTschSF->handleResponse(sender, RC_RESET, 0, emptyCellList);

        /* cancel any pending pattern update that we might have created */
        if (pendingPatternUpdates[sender])
            pendingPatternUpdates[sender]->setDestId(-1);
        return response;
    }

    /* We interrupted an ongoing transaction or got a response to our CLEAR
      => don't need to perform any other checks
      (TODO: Do we still need to check seqNum though? Unclear in the draft!) */
    if (pTschLinkInfo->getLastKnownCommand(sender) == CMD_CLEAR)
    {
        pTschLinkInfo->setLastKnownType(sender, MSG_RESPONSE);
        pTschSF->handleResponse(sender, RC_SUCCESS, 0, emptyCellList); // it's safe to assume CLEAR always succeeds

        /* cancel any pending pattern update that we might have created */
        if (pendingPatternUpdates[sender])
            pendingPatternUpdates[sender]->setDestId(-1);
        return response;
    }

    if (seqNum == pTschLinkInfo->getLastKnownSeqNum(sender)
            && MSG_RESPONSE == pTschLinkInfo->getLastKnownType(sender))
    {
        /* pkt is duplicate, ignore */
        EV_DETAIL << "Received duplicate response, ignoring" << endl;
        return response;
    }

    if (returnCode == RC_SEQNUM) {
        /* we don't currently send/handle those (see comment in handleRequestMsg()) */
        EV_ERROR << "Received unexpected RC_SEQNUM" << endl;
        return response;
    }

    if (seqNum != pTschLinkInfo->getLastKnownSeqNum(sender)) {
        /* schedule inconsistency detected (sequence number!) or other node has
           detected an inconsistency in their schedule with us*/
        pTschSF->handleInconsistency(sender, seqNum);
        return response;
    }

    pTschLinkInfo->setLastKnownType(sender, MSG_RESPONSE);

    auto pl = pkt->popAtBack<BytesChunk>(pkt->getDataLength(), Chunk::PF_ALLOW_NULLPTR);
    if (pl != nullptr) {
        /* there's piggybacked data, let the SF that uses this data handle it */
        EV_DETAIL << "Node " << pNodeId << " handling piggybacked data" << endl;
        pTschSF->handlePiggybackedData(sender,(void*) &pl->getBytes());
    }

    if (returnCode == RC_CELLLIST) {
        /* "In case the received Response Code is RC_ERR_CELLLIST,
            the transaction is aborted and no cell is relocated" */
        pTschLinkInfo->abortTransaction(sender);
        pTschSF->handleResponse(sender, returnCode, 0, emptyCellList);
    } else if (returnCode == RC_SFID) {
        /* the draft doesn't define how to handle this (yet), wait for updates*/
    } else if (returnCode == RC_SUCCESS) {
        std::vector<cellLocation_t> cellList = data->getCellList();
        auto cellOption = pTschLinkInfo->getLastLinkOption(sender);
        tsch6pCmd_t lastCmd = pTschLinkInfo->getLastKnownCommand(sender);
        std::vector<cellLocation_t> deleteCells = {};

        switch (lastCmd) {
            case CMD_ADD:
                pTschLinkInfo->addCells(sender, cellList, cellOption);
                break;
            case CMD_DELETE:
                pTschLinkInfo->deleteCells(sender, cellList, cellOption);
                break;
            case CMD_RELOCATE:
                if (!cellList.size()) {
                    EV_DETAIL << "Received empty SUCCESS for RELOCATE, none of the proposed cells were accepted!" << endl;
                    break;
                }

                deleteCells = pTschLinkInfo->getRelocationCells(sender);
                pTschLinkInfo->relocateCells(sender, cellList, cellOption);
                break;
            default:
                EV_ERROR << "unknown or unexpected command - " << lastCmd << endl;
        }

        /* let SF know the transaction it initiated was a success */
        pTschSF->handleResponse(sender, returnCode, cellList.size(), cellList);

        /* tell mac layer to update hopping patterns
           (we can do this here because we will ACK the message we've just
            handled during the currently active timeframe. all hopping pattern
            updates will only be active after that.) */

        // TODO: Change to directly update TschSlotframe instead of using msg
        tsch6topCtrlMsg *msg = new tsch6topCtrlMsg();
        if (lastCmd == CMD_DELETE)
            setCtrlMsg_PatternUpdate(msg, sender, cellOption, emptyCellList, cellList, data->getTimeout());
        else
            setCtrlMsg_PatternUpdate(msg, sender, cellOption, cellList, deleteCells, data->getTimeout());

        //sendControlDown(msg);
        updateSchedule(*msg);

        /* as far as we're considered, this transaction is complete now. */
        pTschLinkInfo->abortTransaction(sender);
        pTschLinkInfo->incrementSeqNum(sender);
    }


    return response;
}

void Tsch6topSublayer::receiveSignal(cComponent *source, simsignal_t signalID, cObject *value, cObject *details) {
    Enter_Method_Silent();
    bool txSuccess = false;

    if (signalID == inet::packetDroppedSignal)
        txSuccess = false;
    else if (signalID == inet::packetSentSignal)
        txSuccess = true;
    else
        // we cannot process this signal?!
        return;

    auto pkt = dynamic_cast<Packet *>(value);
    if (!pkt)
        return;

    // TODO This does not work, find a better solution to filter unwanted signals
    /**auto proto = pkt->getTag<PacketProtocolTag>();

    if (proto->getProtocol() != &Protocol::wiseRoute) {
        return;
    }**/

    auto machdr = pkt->popAtFront<Ieee802154eMacHeader>();
    uint64_t destId = machdr->getDestAddr().getInt();

    if (machdr->getNetworkProtocol() == -1
            || ProtocolGroup::ethertype.getProtocol(machdr->getNetworkProtocol()) != &Protocol::wiseRoute)
    {
        return;
    }

    auto sixphdr = pkt->popAtFront<tsch::sixtisch::SixpHeader>();

    if ( sixphdr && ((tsch6pMsg_t) sixphdr->getType()) == MSG_REQUEST )
        return;

    tsch6topCtrlMsg* result = NULL;

    bool pigbackDataExists = piggybackableData.find(destId) != piggybackableData.end();
    auto pigbackData = piggybackableData[destId];

    if (pigbackDataExists && !pigbackData.empty() && pigbackData.front()->getInTransit()) {
        if (txSuccess)
            /* successfully piggybacked data, remove it from queue */
            deletePiggybackableData(destId, pigbackData.front()->getContextPointer());
        else {
            /* data not successfully piggybacked, try again */
            // TODO: the whole assumption that it's always the front that has been
            // piggybacked is quite feeble; but time etc etc.
            pigbackData.front()->setInTransit(false);
            if (pigbackData.front()->getArrivalTime() <= simTime())
                /* piggyback timeout already ran out, re-schedule */
                scheduleAt(simTime() + PIGGYBACKING_BACKOFF, pigbackData.front());
        }
    }

    if (pendingPatternUpdates.find(destId) == pendingPatternUpdates.end()) {
        EV_DETAIL << "We are in test condition #1" << endl;
//        if (pTschLinkInfo->inTransaction(destId))
//            pTschLinkInfo->abortTransaction(destId); // EXPERIMENTAL: clear in-transaction flag for TSCH-ACKs on RC_RESET

        return;
    }

    /* txsuccess is for a 6P transmission towards one of our neighbors */
    if (txSuccess && pendingPatternUpdates[destId]->getDestId() != -1
            && pendingPatternUpdates[destId]->getTimeout() > simTime()
            && pTschLinkInfo->inTransaction(destId))
    {
        /* LL ACK arrived and there is a pattern update waiting for this ACK
         * and the transaction hasn't timed out in the meantime */
        EV_DETAIL << "LL ACK arrived and there is a pattern update waiting for this ACK" << endl;

        tsch6topCtrlMsg* pendingPatternUpdate = pendingPatternUpdates[destId];
        auto cellsToAdd = pendingPatternUpdate->getNewCells();
        auto cellsToDelete = pendingPatternUpdate->getDeleteCells();
        auto cellOptions = pendingPatternUpdate->getCellOptions();

        EV_DETAIL << "Cells to add: " << cellsToAdd << "\nto delete: " << cellsToDelete << endl;

        // TODO: using handleResponse() is a bit hacky since we're the ones who
        // sent the response, we're just handling the fact that it was ACKed
        // create dedicated SF fct for that?
        pTschSF->handleResponse(destId, RC_SUCCESS, 0, NULL);
//        pTschLinkInfo->incrementSeqNum(destId);

        // TODO: wenn reloc: cells in reloccellist aus ptschlinkinfo entfernen
        pTschLinkInfo->addCells(destId, cellsToAdd, cellOptions);
        if (cellsToDelete.size())
            pTschLinkInfo->deleteCells(destId, cellsToDelete, cellOptions);

//        /* as far as we're considered, this transaction is complete now. */
//        pTschLinkInfo->abortTransaction(destId);

        result = pendingPatternUpdate->dup();
    }

    if (pTschLinkInfo->inTransaction(destId))
    {
        /* as far as we're considered, this transaction is complete now. */
        // EXPERIMENTAL: moved these 2 lines here to ensure proper seqNum management and transaction termination
        // when receiving an ACK for empty RC_SUCCESS
        pTschLinkInfo->incrementSeqNum(destId);
        pTschLinkInfo->abortTransaction(destId);
    }

    // TODO: this gets called in response to SIGNALs as well. might break stuff.
    /* PatternUpdate is no longer needed */
    pendingPatternUpdates[destId]->setDestId(-1);

    if (result != nullptr) {
        result->setDestId(destId);
        updateSchedule(*result);
    }

    pTschSF->recordPDR(nullptr); // TODO call with meaningful parameters
}

Packet* Tsch6topSublayer::handleTransactionTimeout(tschLinkInfoTimeoutMsg* tom) {
    uint64_t destId = tom->getNodeId();
    uint8_t seqNum = tom->getSeqNum();
    std::vector<cellLocation_t> emptyCellList = {};

    auto lastCmd = pTschLinkInfo->getLastKnownCommand(destId);
    auto lastType = pTschLinkInfo->getLastKnownType(destId);

    EV_DETAIL << "Timeout fired for " << lastCmd << " "
            << lastType << " addressed to " << MacAddress(destId) << endl;

    if (!pTschLinkInfo->linkInfoExists(destId)) {
        std::ostringstream out;
        out << "Received timeout for link with " << MacAddress(destId)
                << " that doesn't exist, something went very wrong";

        throw cRuntimeError(out.str().c_str());
    }

    /* Notify SF that it might want to try again */
    pTschSF->handleTransactionTimeout(destId);

    numTimeouts++;

    return NULL;
}

Packet* Tsch6topSublayer::createAddRequest(uint64_t destId, uint8_t seqNum,
                                       uint8_t cellOptions, int numCells,
                                       std::vector<cellLocation_t>& cellList,
                                       simtime_t timeout)
{
    if (numCells == 0 || timeout <= 0) {
        return NULL;
    }

    int cellListSz = cellHdrSz * cellList.size();

    const auto& sixpHeader = makeShared<tsch::sixtisch::SixpHeader>();
    sixpHeader->setType(MSG_REQUEST);
    sixpHeader->setSeqNum(seqNum);
    sixpHeader->setSfid(pSFID);
    sixpHeader->setCode(CMD_ADD);
    sixpHeader->setChunkLength(B(4));

    const auto& sixpData = makeShared<tsch::sixtisch::SixpData>();
    sixpData->setCellOptions(cellOptions);
    sixpData->setNumCells(numCells);
    sixpData->setCellList(cellList);
    sixpData->setTimeout(timeout);
    sixpData->setChunkLength(b(addDelRelocReqMsgHdrSz + cellListSz + (sizeof(simtime_t)*8)));

    auto pkt = new Packet("6top ADD Req");
    piggybackOnMessage(pkt, destId);
    pkt->insertAtBack(sixpData);
    pkt->insertAtFront(sixpHeader);

    auto macAddressReq = pkt->addTagIfAbsent<MacAddressReq>();
    macAddressReq->setSrcAddress(MacAddress(pNodeId));
    macAddressReq->setDestAddress(MacAddress(destId));

    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::wiseRoute);
    auto virtualTag = pkt->addTagIfAbsent<VirtualLinkTagReq>();
    virtualTag->setVirtualLinkID(-2);
    return pkt;
}

Packet* Tsch6topSublayer::createDeleteRequest(uint64_t destId, uint8_t seqNum,
                            uint8_t cellOptions, int numCells,
                            std::vector<cellLocation_t> cellList,
                            simtime_t timeout)
{
    if (numCells == 0 || timeout <= 0 || numCells > (int)cellList.size())
        return NULL;

    int cellListSz = cellHdrSz * cellList.size();

    const auto& sixpHeader = makeShared<tsch::sixtisch::SixpHeader>();
    sixpHeader->setType(MSG_REQUEST);
    sixpHeader->setSeqNum(seqNum);
    sixpHeader->setSfid(pSFID);
    sixpHeader->setCode(CMD_DELETE);
    sixpHeader->setChunkLength(B(4));

    const auto& sixpData = makeShared<tsch::sixtisch::SixpData>();
    sixpData->setCellOptions(cellOptions);
    sixpData->setNumCells(numCells);
    sixpData->setCellList(cellList);
    sixpData->setTimeout(timeout);
    sixpData->setChunkLength(b(addDelRelocReqMsgHdrSz + cellListSz + (sizeof(simtime_t)*8)));

    auto pkt = new Packet("6top DEL Req");
    piggybackOnMessage(pkt, destId);
    pkt->insertAtBack(sixpData);
    pkt->insertAtFront(sixpHeader);

    auto macAddressReq = pkt->addTagIfAbsent<MacAddressReq>();
    macAddressReq->setSrcAddress(MacAddress(pNodeId));
    macAddressReq->setDestAddress(MacAddress(destId));

    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::wiseRoute);
    auto virtualTag = pkt->addTagIfAbsent<VirtualLinkTagReq>();
    virtualTag->setVirtualLinkID(-2);
    return pkt;
}

Packet* Tsch6topSublayer::createRelocationRequest(uint64_t destId, uint8_t seqNum,
                            uint8_t cellOptions, int numCells,
                            std::vector<cellLocation_t> &relocationCellList,
                            std::vector<cellLocation_t> &candiateCellList,
                            simtime_t timeout) {
    if (numCells == 0 || timeout <= 0 || numCells != (int)relocationCellList.size()) {
        return NULL;
    }

    /* each cell takes up 4 bytes in the packet */
    int cellListSz = cellHdrSz * relocationCellList.size()
                     + cellHdrSz * candiateCellList.size();

    const auto& sixpHeader = makeShared<tsch::sixtisch::SixpHeader>();
    sixpHeader->setType(MSG_REQUEST);
    sixpHeader->setSeqNum(seqNum);
    sixpHeader->setSfid(pSFID);
    sixpHeader->setCode(CMD_RELOCATE);
    sixpHeader->setChunkLength(B(4));

    const auto& sixpData = makeShared<tsch::sixtisch::SixpData>();
    sixpData->setCellOptions(cellOptions);
    sixpData->setNumCells(numCells);
    sixpData->setCellList(candiateCellList);
    sixpData->setRelocationCellList(relocationCellList);
    sixpData->setTimeout(timeout);
    sixpData->setChunkLength(b(addDelRelocReqMsgHdrSz + cellListSz + (sizeof(simtime_t)*8)));

    auto pkt = new Packet("6top RELOCATE Req");
    piggybackOnMessage(pkt, destId);
    pkt->insertAtBack(sixpData);
    pkt->insertAtFront(sixpHeader);

    auto macAddressReq = pkt->addTagIfAbsent<MacAddressReq>();
    macAddressReq->setSrcAddress(MacAddress(pNodeId));
    macAddressReq->setDestAddress(MacAddress(destId));

    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::wiseRoute);
    auto virtualTag = pkt->addTagIfAbsent<VirtualLinkTagReq>();
        virtualTag->setVirtualLinkID(-2);
    return pkt;
}

Packet* Tsch6topSublayer::createClearRequest(uint64_t destId, uint8_t seqNum) {

    const auto& sixpHeader = makeShared<tsch::sixtisch::SixpHeader>();
    sixpHeader->setType(MSG_REQUEST);
    sixpHeader->setSeqNum(seqNum);
    sixpHeader->setSfid(pSFID);
    sixpHeader->setCode(CMD_CLEAR);
    sixpHeader->setChunkLength(B(4));

    const auto& sixpData = makeShared<tsch::sixtisch::SixpData>();
    sixpData->setTimeout(simTime() + TxQueueTTL);
    sixpData->setChunkLength(b(baseMsgHdrSz + 16));

    auto pkt = new Packet("6top CLEAR Req");
    pkt->insertAtBack(sixpData);
    pkt->insertAtFront(sixpHeader);

    auto macAddressReq = pkt->addTagIfAbsent<MacAddressReq>();
    macAddressReq->setSrcAddress(MacAddress(pNodeId));
    macAddressReq->setDestAddress(MacAddress(destId));

    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::wiseRoute);
    auto virtualTag = pkt->addTagIfAbsent<VirtualLinkTagReq>();
        virtualTag->setVirtualLinkID(-2);
    return pkt;
}

Packet* Tsch6topSublayer::createSignalRequest(uint64_t destId, uint8_t seqNum,
                                                void* payload, int payloadSz) {

    const auto& sixpHeader = makeShared<tsch::sixtisch::SixpHeader>();
    sixpHeader->setType(MSG_REQUEST);
    sixpHeader->setSeqNum(seqNum);
    sixpHeader->setSfid(pSFID);
    sixpHeader->setCode(CMD_SIGNAL);
    sixpHeader->setChunkLength(B(4));

    const auto& sixpData = makeShared<tsch::sixtisch::SixpData>();
    sixpData->setTimeout(simTime() + TxQueueTTL);
    sixpData->setChunkLength(B(payloadSz + sizeof(simtime_t)));

    auto pkt = new Packet("6top SIGNAL Req");

    const auto& payloadCast = static_cast<uint8_t *>(payload);

    const auto& payloadChunk = makeShared<BytesChunk>(payloadCast, payloadSz);

    pkt->insertAtBack(payloadChunk);
    pkt->insertAtBack(sixpData);
    pkt->insertAtFront(sixpHeader);

    auto macAddressReq = pkt->addTagIfAbsent<MacAddressReq>();
    macAddressReq->setSrcAddress(MacAddress(pNodeId));
    macAddressReq->setDestAddress(MacAddress(destId));

    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::wiseRoute);
    auto virtualTag = pkt->addTagIfAbsent<VirtualLinkTagReq>();
        virtualTag->setVirtualLinkID(-2);
    return pkt;
}

Packet* Tsch6topSublayer::createSuccessResponse(uint64_t destId, uint8_t seqNum,
                                        std::vector<cellLocation_t> &cellList,
                                        simtime_t timeout)
{
    int cellListSz = cellHdrSz * cellList.size();

    const auto& sixpHeader = makeShared<tsch::sixtisch::SixpHeader>();
    sixpHeader->setType(MSG_RESPONSE);
    sixpHeader->setSeqNum(seqNum);
    sixpHeader->setSfid(pSFID);
    sixpHeader->setCode(RC_SUCCESS);
    sixpHeader->setChunkLength(B(4));

    const auto& sixpData = makeShared<tsch::sixtisch::SixpData>();
    sixpData->setCellList(cellList);
    sixpData->setTimeout(timeout);
    sixpData->setChunkLength(b(cellListSz + sizeof(simtime_t)*8));

    auto pkt = new Packet("6top SUCCESS Resp");
    piggybackOnMessage(pkt, destId);
    pkt->insertAtBack(sixpData);
    pkt->insertAtFront(sixpHeader);

    auto macAddressReq = pkt->addTagIfAbsent<MacAddressReq>();
    macAddressReq->setSrcAddress(MacAddress(pNodeId));
    macAddressReq->setDestAddress(MacAddress(destId));

    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::wiseRoute);
    auto virtualTag = pkt->addTagIfAbsent<VirtualLinkTagReq>();
    virtualTag->setVirtualLinkID(-2);
    return pkt;
}

Packet* Tsch6topSublayer::createErrorResponse(uint64_t destId, uint8_t seqNum,
                                          tsch6pReturn_t returnCode,
                                          simtime_t timeout)
{
    if (returnCode < RC_ERROR || returnCode == RC_SEQNUM )
        /* returnCode isn't an error code, abort. */
        return NULL;

    const auto& sixpHeader = makeShared<tsch::sixtisch::SixpHeader>();
    sixpHeader->setType(MSG_RESPONSE);
    sixpHeader->setSeqNum(seqNum);
    sixpHeader->setSfid(pSFID);
    sixpHeader->setCode(returnCode);
    sixpHeader->setChunkLength(B(4));

    const auto& sixpData = makeShared<tsch::sixtisch::SixpData>();
    sixpData->setTimeout(timeout);
    sixpData->setChunkLength(B(sizeof(simtime_t)));

    auto pkt = new Packet("6top ERROR Resp");
    pkt->insertAtBack(sixpData);
    pkt->insertAtFront(sixpHeader);

    auto macAddressReq = pkt->addTagIfAbsent<MacAddressReq>();
    macAddressReq->setSrcAddress(MacAddress(pNodeId));
    macAddressReq->setDestAddress(MacAddress(destId));

    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::wiseRoute);
    auto virtualTag = pkt->addTagIfAbsent<VirtualLinkTagReq>();
        virtualTag->setVirtualLinkID(-2);
    return pkt;
}

Packet* Tsch6topSublayer::createSeqNumErrorResponse(uint64_t destId,
                                                          uint8_t seqNum,
                                                          simtime_t timeout) {
    const auto& sixpHeader = makeShared<tsch::sixtisch::SixpHeader>();
    sixpHeader->setType(MSG_RESPONSE);
    sixpHeader->setSeqNum(seqNum);
    sixpHeader->setSfid(pSFID);
    sixpHeader->setCode(RC_SEQNUM);
    sixpHeader->setChunkLength(B(4));

    const auto& sixpData = makeShared<tsch::sixtisch::SixpData>();
    sixpData->setTimeout(timeout);
    sixpData->setChunkLength(B(sizeof(simtime_t)));

    auto pkt = new Packet("6top SEQ ERROR Resp");
    pkt->insertAtBack(sixpData);
    pkt->insertAtFront(sixpHeader);

    auto macAddressReq = pkt->addTag<MacAddressReq>();
    macAddressReq->setSrcAddress(MacAddress(pNodeId));
    macAddressReq->setDestAddress(MacAddress(destId));

    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::wiseRoute);
    auto virtualTag = pkt->addTagIfAbsent<VirtualLinkTagReq>();
        virtualTag->setVirtualLinkID(-2);
    return pkt;
}

Packet* Tsch6topSublayer::createClearResponse(uint64_t destId, uint8_t seqNum,
                                                    tsch6pReturn_t returnCode,
                                                    simtime_t timeout)
{
    if (returnCode != RC_SUCCESS && returnCode != RC_RESET) {
        /* returnCode isn't valid, abort. */
        return NULL;
    }

    const auto& sixpHeader = makeShared<tsch::sixtisch::SixpHeader>();
    sixpHeader->setType(MSG_RESPONSE);
    sixpHeader->setSeqNum(seqNum);
    sixpHeader->setSfid(pSFID);
    sixpHeader->setCode(returnCode);
    sixpHeader->setChunkLength(B(4));

    const auto& sixpData = makeShared<tsch::sixtisch::SixpData>();
    sixpData->setTimeout(timeout);
    sixpData->setChunkLength(B(sizeof(simtime_t)));

    auto pkt = new Packet("6top CLEAR Resp");
    pkt->insertAtBack(sixpData);
    pkt->insertAtFront(sixpHeader);

    auto macAddressReq = pkt->addTag<MacAddressReq>();
    macAddressReq->setSrcAddress(MacAddress(pNodeId));
    macAddressReq->setDestAddress(MacAddress(destId));

    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::wiseRoute);
    auto virtualTag = pkt->addTagIfAbsent<VirtualLinkTagReq>();
        virtualTag->setVirtualLinkID(-2);
    return pkt;
}

// TODO: In later versions it can be changed to a struct or something similar, since this message will never be sent
tsch6topCtrlMsg* Tsch6topSublayer::setCtrlMsg_PatternUpdate(
                            tsch6topCtrlMsg* msg,
                            uint64_t destId,
                            uint8_t cellOption,
                            std::vector<cellLocation_t> newCells,
                            std::vector<cellLocation_t> deleteCells,
                            simtime_t timeout)
{
    if (!msg)
        msg = new tsch6topCtrlMsg();

    msg->setKind(CTRLMSG_PATTERNUPDATE);
    msg->setDestId(destId);
    msg->setCellOptions(cellOption);
    msg->setNewCells(newCells);
    msg->setDeleteCells(deleteCells);
    msg->setTimeout(timeout);

    return msg;
}

void Tsch6topSublayer::updateSchedule(tsch6topCtrlMsg msg) {
    auto dest = msg.getDestId();

    if (!dest) {
        std::ostringstream out;
        out << "Trying to update the schedule but MAC address of the neighbor on this link is not specified! - "
                << dest << "\ncells to add: " << msg.getNewCells() << "\nto delete: " << msg.getDeleteCells() << endl;
        throw cRuntimeError(out.str().c_str());
    }


    for (cellLocation_t cell : msg.getNewCells()) {
        TschLink *tl = schedule->createLink();
        tl->setAddr(inet::MacAddress(msg.getDestId()));
        uint8_t cellOption = msg.getCellOptions();
        tl->setShared(getCellOptions_isSHARED(cellOption));
        tl->setRx(getCellOptions_isRX(cellOption));
        tl->setTx(getCellOptions_isTX(cellOption));
        tl->setAuto(getCellOptions_isAUTO(cellOption));
        tl->setChannelOffset(cell.channelOffset);
        tl->setSlotOffset(cell.timeOffset);
        schedule->addLink(tl);
    }

    for (auto cell : msg.getDeleteCells())
    {
        if ( !schedule->removeLinkAtCell(cell, dest) )
            EV_DETAIL << "Link at " << cell << " doesn't exist" << endl;
        else
            EV_DETAIL << "Link at " << cell << " deleted" << endl;
    }
}


void Tsch6topSublayer::setCellOption(uint8_t* cellOptions, uint8_t option) {
    Enter_Method_Silent();

    *cellOptions = *cellOptions | option;
}

uint8_t Tsch6topSublayer::invertCellOption(uint8_t senderOpt) {
    uint8_t myOpt = MAC_LINKOPTIONS_SHARED;
    if (senderOpt != MAC_LINKOPTIONS_SHARED) {
        if (senderOpt == MAC_LINKOPTIONS_TX)
            myOpt = MAC_LINKOPTIONS_RX;
        else
            myOpt = MAC_LINKOPTIONS_TX;
    }

    return myOpt;
}


void Tsch6topSublayer::handleLowerControl(cMessage * msg) {
    EV_DETAIL << "Tsch6topSublayer got lowerCtrlMsg" << std::endl;
/**    WaicMacPktTxIndicationCtrlMsg* TxMsg = (WaicMacPktTxIndicationCtrlMsg*) msg;
    if (TxMsg->getNodeId() == 0) {
        // TODO delete this?!
        std::cout << "xxxx Tsch6topSublayer got TxMsg!! xxxx" << std::endl;
    }**/
    //delete msg; //delete message which is not addressed for this app
}

bool Tsch6topSublayer::getCellOptions_isMultiple(uint8_t cellOptions) {
    bool isTX = getCellOptions_isTX(cellOptions);
    bool isRX = getCellOptions_isRX(cellOptions);
    bool isSHARED = getCellOptions_isSHARED(cellOptions);

    return !(isTX != isRX != isSHARED != (isTX && isRX && isSHARED));
}

uint8_t Tsch6topSublayer::getCellOption(uint8_t cellOptions) {
    if (getCellOptions_isTX(cellOptions)) {
        return MAC_LINKOPTIONS_TX;
    }
    if (getCellOptions_isRX(cellOptions)) {
        return MAC_LINKOPTIONS_RX;
    }
    return MAC_LINKOPTIONS_SHARED;
}

simtime_t Tsch6topSublayer::getAbsoluteTimeout(int timeout) {
    /* since simtime_t is always counted in (fractions of) seconds,
       we need to convert timeout first */
    return simTime() + SimTime(timeout, SIMTIME_MS);
}

uint8_t Tsch6topSublayer::prepLinkForRequest(uint64_t destId, simtime_t absoluteTimeout) {
    uint8_t seqNum = 0;

    if (!pTschLinkInfo->linkInfoExists(destId)) {
        /* We don't have a link to destId yet, register one and start @ SeqNum 0 */
        EV_INFO << "We don't have a link to " << MacAddress(destId) << " yet, register one and start at seqNum 0" << endl;
        pTschLinkInfo->addLink(destId, true, absoluteTimeout, seqNum);
    } else {
        seqNum = pTschLinkInfo->getSeqNum(destId);
        EV_INFO << "Found link to " << MacAddress(destId) << " with seqNum " << +seqNum << endl;
        /* Link already exists, update its info */
        pTschLinkInfo->setInTransaction(destId, absoluteTimeout);
    }

    pTschLinkInfo->setLastKnownType(destId, MSG_REQUEST);

    return seqNum;
}

void Tsch6topSublayer::piggybackOnMessage(Packet* pkt, uint64_t destId) {
    if (piggybackableData.find(destId) == piggybackableData.end())
        return;

    auto pigbackData = piggybackableData[destId];

    /* vector for destId exists: there might be data to piggyback */
    for (auto it = pigbackData.begin(); it != pigbackData.end(); ++it) {
        if ((*it)->getInTransit())
            continue;

        /* found data to piggyback, put it in our packet */
        const auto& payloadCast = static_cast<uint8_t *>((*it)->getContextPointer());
        const auto& payloadChunk = makeShared<BytesChunk>(payloadCast, ( ((*it)->getPayloadSz()) / 8) );

        (*it)->setInTransit(true);
        pkt->insertAtBack(payloadChunk);

        break;
    }
}

// TODO: remove this and just use erase nstead?! piggybackabledata now just contains
// the msg holding all info anyway...
void Tsch6topSublayer::deletePiggybackableData(uint64_t destId, void* payloadPtr) {
    if (piggybackableData.find(destId) != piggybackableData.end()) {
        std::vector<tsch6pPiggybackTimeoutMsg*>::iterator i;
        for(i = piggybackableData[destId].begin();
            i != piggybackableData[destId].end(); ++i) {
            if ((*i)->getContextPointer() == payloadPtr) {
                cancelAndDelete(*i);
                piggybackableData[destId].erase(i);
                break;
            }
        }
    }
}

std::list<uint64_t> Tsch6topSublayer::getNeighborsInRange(uint64_t nodeId, Ieee802154eMac* mac){
    std::list<uint64_t> resultingList;
    auto medium = dynamic_cast<const physicallayer::RadioMedium*>(mac->getRadio()->getMedium());
    auto limitcache = dynamic_cast<const physicallayer::MediumLimitCache*>(medium->getMediumLimitCache());
    auto range = limitcache->getMaxCommunicationRange(mac->getRadio()).get();
    auto myCoords = mac->getRadio()->getAntenna()->getMobility()->getCurrentPosition();

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
        if (interfaceModule->getNumInterfaces() > 0) {
            for (int y = 0; y < interfaceModule->getNumInterfaces(); y++) {
                if (strcmp(interfaceModule->getInterface(y)->getNedTypeName(), "inet.linklayer.common.WirelessInterface") == 0) {
                    addr = interfaceModule->getInterface(y)->getMacAddress();
                    break;
                }
            }
        }

        // we found no mac address to use or we found ourself
        if (addr.isUnspecified() || addr.getInt() == nodeId) {
            continue;
        }

        // verify distance to ourself
        if (myCoords.distance(coords) <= range) {
            resultingList.push_back(addr.getInt());
            EV_DETAIL << "node " << addr.str() << " (" << coords.str() << ") is a neighbor of " << MacAddress(nodeId).str() << " (" << myCoords.str() << ")" << endl;
        }
    }

    return resultingList;
}
