/*
 * Minimal Scheduling Function Implementation (6TiSCH WG Draft).
 *
 * Copyright (C) 2021  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2021  Yevhenii Shudrenko
 *           (C) 2019  Leo Krüger
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "TschMSF.h"
#include "RplDefs.h"
#include "inet/common/ModuleAccess.h"
#include "Tsch6tischComponents.h"
#include "../Ieee802154eMac.h"
#include "../TschVirtualLink.h"
#include <omnetpp.h>
#include <random>
#include <algorithm>
#include <queue>
#include <chrono>

using namespace tsch;
using namespace std;

Define_Module(TschMSF);

TschMSF::TschMSF() :
    rplParentId(0),
    tsch6pRtxThresh(3),
    numInconsistencies(0),
    numLinkResets(0),
    hasStarted(false),
    pHousekeepingDisabled(false),
    hasOverlapping(false),
    isSink(false),
    delayed6pReq(nullptr),
    numFailedTracked6p(0),
    num6pAddSent(0),
    uplinkSlotOffset(0),
    num6pAddFailed(0),
    uplinkCellUtil(0),
    numUnhandledResponses(0),
    showLinkResets(false),
    pCheckScheduleConsistency(false),
    isLeafNode(true),
    numCellsRequired(-1),
    numTranAbandonedMaxRetries(0),
    neighbors({})
{
}
TschMSF::~TschMSF() {
}

void TschMSF::initialize(int stage) {
    if (stage == 0) {
        EV_DETAIL << "MSF initializing" << endl;
        pNumChannels = getModuleByPath("^.^.^.^.^.channelHopping")->par("nbRadioChannels").intValue();
        pTschLinkInfo = (TschLinkInfo*) getParentModule()->getSubmodule("linkinfo");
        pMaxNumCells = par("maxNumCells");
        pMaxNumTx = par("maxNumTx");
        pTimeout = par("timeout").intValue();
        pLimNumCellsUsedHigh = par("upperCellUsageLimit");
        pLimNumCellsUsedLow = par("lowerCellUsageLimit");
        pRelocatePdrThres = par("relocatePdrThresh");
        pCellListRedundancy = par("cellListRedundancy").intValue();
        pNumMinimalCells = par("numMinCells").intValue();
        isDisabled = par("disable").boolValue();
        pCellIncrement = par("cellsToAdd").intValue();
        pSend6pDelayed = par("send6pDelayed").boolValue();
        pHousekeepingPeriod = par("housekeepingPeriod").intValue();
        pHousekeepingDisabled = par("disableHousekeeping").boolValue();
        pCheckScheduleConsistency = par("checkScheduleConsistency").boolValue();
        pCellBundlingEnabled = par("cellBundlingEnabled").boolValue();
        pCellBundleSize = par("cellBundleSize").intValue();
        queueUtilization = registerSignal("queueUtilization");
        failed6pAdd = registerSignal("failed6pAdd");
        uplinkScheduledSignal = registerSignal("uplinkScheduled");
        uncoverableGapSignal = registerSignal("uncoverableGap");
    } else if (stage == 5) {
        interfaceModule = dynamic_cast<InterfaceTable *>(getParentModule()->getParentModule()->getParentModule()->getParentModule()->getSubmodule("interfaceTable", 0));
        pNodeId = interfaceModule->getInterface(1)->getMacAddress().getInt();
        pSlotframeLength = getModuleByPath("^.^.schedule")->par("macSlotframeSize").intValue();
        pTsch6p = (Tsch6topSublayer*) getParentModule()->getSubmodule("sixtop");
        mac = check_and_cast<Ieee802154eMac*>(getModuleByPath("^.^.mac"));
        mac->subscribe(POST_MODEL_CHANGE, this);
        mac->subscribe(mac->pktRecFromUpperSignal, this);
        mac->subscribe(mac->pktRecFromLowerSignal, this);

        if (par("lowLatencyMode").boolValue())
            mac->subscribe(linkBrokenSignal, this);

        schedule = check_and_cast<TschSlotframe*>(getModuleByPath("^.^.schedule"));
        hopping = check_and_cast<TschHopping*>(getContainingNode(this)->getParentModule()->getSubmodule("channelHopping"));

        if (isDisabled)
            return;

        hostNode = getModuleByPath("^.^.^.^.");
        showTxCells = par("showDedicatedTxCells").boolValue();
        showQueueUtilization = par("showQueueUtilization").boolValue();
        showTxCellCount = par("showTxCellCount").boolValue();
        showLinkResets = par("showLinkResets").boolValue();
        showQueueSize = par("showQueueSize").boolValue();

        /** Schedule minimal cells for broadcast control traffic [RFC8180, 4.1] */
        scheduleMinimalCells(pNumMinimalCells, pSlotframeLength);

        /** And an auto RX cell for communication with neighbors [IETF MSF draft, 3] */
        scheduleAutoRxCell(interfaceModule->getInterface(1)->getMacAddress().formInterfaceIdentifier());

//        delayed6pReq = new cMessage("SEND_6P_DELAYED", SEND_6P_REQ);

        WATCH(numInconsistencies);
        WATCH(numLinkResets);
        WATCH(num6pAddSent);
        WATCH(uplinkSlotOffset);
        WATCH(pMaxNumCells);
        WATCH(rplParentId);
        WATCH(isLeafNode);
        WATCH(rplRank);
        WATCH(num6pAddFailed);
        WATCH(pLimNumCellsUsedLow);
        WATCH(pLimNumCellsUsedHigh);
        WATCH_PTRMAP(nbrStatistic);
        WATCH_MAP(cellStatistic);
        WATCH(numCellsRequired);
        WATCH(numTranAbandonedMaxRetries);
        WATCH(numTranAbortedUnknownReason);
        WATCH_MAP(downlinkRequested);
        WATCH_MAP(retryInfo);
        WATCH_LIST(neighbors);

//        WATCH_MAP(reservedTimeOffsets);

        rpl = hostNode->getSubmodule("rpl");

        isSink = rpl ? rpl->par("isRoot") : false;

        if (rpl) {
            rpl->subscribe("parentChanged", this);
            rpl->subscribe("rankUpdated", this);
            rpl->subscribe("childJoined", this);
            rpl->subscribe("tschScheduleDownlink", this);
            rpl->subscribe("tschScheduleUplink", this);
            if (par("lowLatencyMode").boolValue())
                rpl->subscribe("uplinkSlotOffset", this);
        }
    }
}

void TschMSF::scheduleAutoRxCell(InterfaceToken euiAddr) {
    if (!pNumChannels)
        return;
    auto ctrlMsg = new tsch6topCtrlMsg();
    ctrlMsg->setDestId(pNodeId);
    ctrlMsg->setCellOptions(MAC_LINKOPTIONS_RX | MAC_LINKOPTIONS_SRCAUTO);

    // TODO: check this saxHash in more detail, often produces overlapping cells
//    autoRxCellslotOffset = 1 + saxHash(pSlotframeLen - 1, euiAddr);
//    uint32_t autoRxCellchanOffset = saxHash(16, euiAddr);
    std::vector<cellLocation_t> cellList;
//    cellList.push_back({autoRxCellslotOffset, autoRxCellchanOffset});

    autoRxCell = {
            euiAddr.low() % pSlotframeLength,
            static_cast<offset_t>((par("useHashChOffsets").boolValue() ? myhash(euiAddr.low()) : euiAddr.low()) % pNumChannels)
    };

    if ( hopping->isBlacklisted(autoRxCell.channelOffset) )
    {
        auto newChOf = hopping->shiftBlacklisted(autoRxCell.channelOffset, autoRxCell.timeOffset);
        EV_DETAIL << "Auto RX channel offset " << autoRxCell.channelOffset
                << " is blacklisted, shifting to " << newChOf << endl;
        autoRxCell.channelOffset = newChOf;
    }

    recordScalar("autoRxChOffset", (int) autoRxCell.channelOffset);

    EV_DETAIL << "euiAddr: " << euiAddr.low() << ", MAC id: " << pNodeId
            << ", pNumChannels: " << pNumChannels << endl;

    cellList.push_back(autoRxCell);
    ctrlMsg->setNewCells(cellList);
    EV_DETAIL << "Scheduling auto RX cell at " << cellList << endl;

    // Check if auto RX overlaps with minimal cell
    auto overlappingMinCells = pTschLinkInfo->getMinimalCells(autoRxCell.timeOffset);
    if (overlappingMinCells.size()) {
        EV_DETAIL << "Auto RX cell conflicts with minimal cell at " << autoRxCell.timeOffset
                << " slotOffset, deleting this minimal cell to avoid scheduling issues" << endl;

        auto ctrlDelete = new tsch6topCtrlMsg();
        ctrlDelete->setDeleteCells(overlappingMinCells);
        ctrlDelete->setDestId(MacAddress::BROADCAST_ADDRESS.getInt());
        ctrlDelete->setCellOptions(MAC_LINKOPTIONS_RX | MAC_LINKOPTIONS_TX | MAC_LINKOPTIONS_SHARED);
        pTsch6p->updateSchedule(*ctrlDelete);
    }

    pTsch6p->updateSchedule(*ctrlMsg);
    delete ctrlMsg;
}

void TschMSF::scheduleAutoCell(uint64_t neighborId) {
    if (neighborId == 0)
        return;

    EV_DETAIL << "Trying to schedule an auto cell to " << MacAddress(neighborId) << endl;

    // Although this is always supposed to be a single cell,
    // implementation-wise it's easier to keep it as vector
    auto sharedCells = pTschLinkInfo->getSharedCellsWith(neighborId);

    if (sharedCells.size()) {
        auto sharedCellLoc = sharedCells.back();
        if (schedule->getLinkByCellCoordinates(sharedCellLoc.timeOffset, sharedCellLoc.channelOffset, MacAddress(neighborId)))
        {
            EV_DETAIL << "Already scheduled, aborting" << endl;
            return;
        }

        throw cRuntimeError("Shared auto cell found in TschLinkInfo but missing in the schedule!");
    }

    auto ctrlMsg = new tsch6topCtrlMsg();
    ctrlMsg->setDestId(neighborId);
    ctrlMsg->setCellOptions(MAC_LINKOPTIONS_TX | MAC_LINKOPTIONS_SHARED | MAC_LINKOPTIONS_SRCAUTO);

    auto neighborIntfIdent = MacAddress(neighborId).formInterfaceIdentifier();

    uint32_t nbruid = neighborIntfIdent.low();

    std::vector<cellLocation_t> cellList;
//    cellList.push_back({1 + saxHash(pSlotframeLen - 1, neighborIntfIdent), saxHash(16, neighborIntfIdent)});

    auto slotOffset = nbruid % pSlotframeLength;
    auto chOffset = (par("useHashChOffsets").boolValue() ? myhash(nbruid) : nbruid) % pNumChannels;

    if ( hopping->isBlacklisted(chOffset) )
    {
        auto newChOf = hopping->shiftBlacklisted(chOffset, slotOffset);
        EV_DETAIL << "Auto TX channel offset " << autoRxCell.channelOffset
                << " is blacklisted, shifting to " << newChOf << endl;
        chOffset = newChOf;
    }

    cellList.push_back({slotOffset, static_cast<offset_t>(chOffset)});
    ctrlMsg->setNewCells(cellList);

    EV_DETAIL << "Scheduling auto TX cell at " << cellList.back() << " to " << MacAddress(neighborId) << endl;

    pTschLinkInfo->addLink(neighborId, false, 0, 0);
    pTschLinkInfo->addCell(neighborId, cellList.back(), MAC_LINKOPTIONS_TX | MAC_LINKOPTIONS_SHARED | MAC_LINKOPTIONS_SRCAUTO);
    pTsch6p->updateSchedule(*ctrlMsg);
    delete ctrlMsg;
}

void TschMSF::finish() {
    if (rplParentId) {
        auto dedCells = pTschLinkInfo->getDedicatedCells(rplParentId);

        recordScalar("numUplinkCells", (int) dedCells.size());

        if ((int) dedCells.size() == 1)
            recordScalar("uplinkSlotOffset", dedCells.back().timeOffset);
        else if (!dedCells.size())
            recordScalar("uplinkSlotOffset", 0);
    }
    recordScalar("numInconsistenciesDetected", numInconsistencies);
    recordScalar("numLinkResets", numLinkResets);
    recordScalar("numUnhandledResponses", numUnhandledResponses);

    if (par("trackFailed6pAddByNum").intValue() > 0)
        recordScalar("tracked6pFailed", numFailedTracked6p);

    if (rplParentId)
        recordScalar("rxCellCoverageRatio", getCoverageRate());
}

void TschMSF::start() {
    Enter_Method_Silent();

    if (isDisabled)
        return;

    tsch6topCtrlMsg* msg = new tsch6topCtrlMsg();
    msg->setKind(DO_START);

    scheduleAt(simTime() + SimTime(par("startTime").doubleValue()), msg);
//    scheduleAt(simTime() + 500, new cMessage("Test cell matching", CHANGE_SLOF));
}

std::string TschMSF::printCellUsage(std::string neighborMac, double usage) {
    std::string usageInfo = std::string("Cell utilization with ") + neighborMac
            + std::string(usage >= pLimNumCellsUsedHigh ? " exceeds upper" : "")
            + std::string(usage <= pLimNumCellsUsedLow ? " is below lower" : "")
            + std::string((usage < pLimNumCellsUsedHigh && usage > pLimNumCellsUsedLow) ? " within" : "")
            + std::string(" limit with ") + std::to_string(usage);

    return usageInfo;
//    std::cout << pNodeId << usageInfo << endl;
}

bool TschMSF::slOfScheduled(offset_t slOf) {
    auto links = schedule->getLinks();
    for (auto l : links)
        if (slOf == l->getSlotOffset())
            return true;
    return false;
}

void TschMSF::handleMaxCellsReached(cMessage* msg) {
    auto neighborMac = (MacAddress*) msg->getContextPointer();
    auto nbrId = neighborMac->getInt();

    // we might get a notification from our own mac
    if (nbrId == pNodeId)
        return;

    EV_DETAIL << "MAX_NUM_CELLS reached, assessing cell usage with " << *neighborMac
            << ", currently scheduled TX cells: " << pTschLinkInfo->getDedicatedCells(nbrId) << endl;

    auto usage = (double) nbrStatistic[nbrId]->numCellsUsed / nbrStatistic[nbrId]->numCellsElapsed;

    if (nbrId == rplParentId)
        uplinkCellUtil = usage;

    EV_DETAIL << printCellUsage(neighborMac->str(), usage) << endl;

    // Avoid attempts to schedule more cells if the lossy-link imitation has started
    if (usage >= pLimNumCellsUsedHigh && !isLossyLink())
    {

          // To avoid schedule congestion in ISM scenarios with seatbelts
//        if (!par("scheduleUplinkOnJoin").boolValue() && isLeafNode && rplRank == 2)
//            return;

        addCells(nbrId, pCellIncrement, MAC_LINKOPTIONS_TX);
    }
    // refrain from deleting downlink cells provisioned on purpose
    else if (usage <= pLimNumCellsUsedLow && downlinkRequested.find(nbrId) != downlinkRequested.end())
        deleteCells(nbrId, 1);

    // reset values
    nbrStatistic[nbrId]->numCellsUsed = 0; //intrand(pMaxNumCells >> 1);
    nbrStatistic[nbrId]->numCellsElapsed = 0;
}

bool TschMSF::isLossyLink() {
    return mac->par("pLinkCollision").doubleValue() > 0
            && simTime().dbl() > mac->par("lossyLinkTimeout").doubleValue();
}

bool TschMSF::addCells(SfControlInfo *retryInfo)
{
    auto numCells = retryInfo->getNumCells();
    auto nodeId = retryInfo->getNodeId();
    if (!pTschLinkInfo->inTransaction(nodeId)) {
        EV_DETAIL << "Seems we can send 6P ADD" << endl;
        return addCells(nodeId, numCells, retryInfo->getCellOptions());
    }

    EV_WARN << "Can't add " << numCells << " to " << MacAddress(nodeId)
            << ", currently in transaction with this node" << endl;
    retryInfo->incRtxCtn();
    if (retryInfo->getRtxCtn() > par("maxRetries").intValue()) {
        EV_DETAIL << "Maximum number of retransmits attempted, dropping packet" << endl;
        delete retryInfo;
        return false;
    }
    auto backoff = pow(4, retryInfo->getRtxCtn());
    auto retryMsg = new cMessage("Retry self-msg", SEND_6P_REQ);
    retryMsg->setControlInfo(retryInfo);
    auto timeout = simTime() + SimTime(backoff, SIMTIME_S);
    scheduleAt(timeout, retryMsg);
    EV_DETAIL << "Another transmission attempt scheduled at " << timeout
            << ", " << retryInfo->getRtxCtn() << " retry" << endl;

    return false;
}


void TschMSF::handleDoStart(cMessage* msg) {
    hasStarted = true;
    EV_DETAIL << "MSF has started" << endl;

    if (!pHousekeepingDisabled)
        scheduleAt(simTime() + par("housekeepingStart"), new tsch6topCtrlMsg("", HOUSEKEEPING));
}

void TschMSF::handleHousekeeping(cMessage* msg) {
    EV_DETAIL << "Performing housekeeping: " << endl;
    scheduleAt(simTime()+ uniform(1, 1.25) * SimTime(pHousekeepingPeriod, SIMTIME_S), msg);

    // iterate over all neighbors
    for (auto const& neighbourId : neighbors) {
        std::map<cellLocation_t, double> pdrStat;

        // calc cell PDR per neighbor
        for (auto cell : pTschLinkInfo->getDedicatedCells(neighbourId)) {
            auto it = cellStatistic.find(cell);
            if (it == cellStatistic.end())
                continue;

            double cellPdr = -1;

            if ((int) std::get<1>(*it).NumTx > 0)
                cellPdr = static_cast<double>(std::get<1>(*it).NumTxAck)
                    / static_cast<double>(std::get<1>(*it).NumTx);

            pdrStat.insert({cell, cellPdr});
        }

        if ((int) pdrStat.size() <= 1)
            continue;

        auto maxPdr = std::max_element(pdrStat.begin(), pdrStat.end(),
                [] (decltype(pdrStat)::value_type a, decltype(pdrStat)::value_type b)
            {
                return a.second < b.second;
            }
        );

//        if (!std::isnan(maxPdr->second))
//            std::cout << "neighbor " << entry.first << " has max pdr " << maxPdr->second << " at "
//                    << maxPdr->first << endl;

        for (auto & cellPdr: pdrStat) {
            if (cellPdr.first == maxPdr->first)
                continue;

//            if (!std::isnan(cellPdr.second))
//                std::cout << " and " << cellPdr.second << " at " << cellPdr.first << endl;

            if (cellPdr.second >= 0 && (maxPdr->second - cellPdr.second) > pRelocatePdrThres) {
                EV_DETAIL << "Cell " << cellPdr.first << "has PDR of " << cellPdr.second * 100
                        << "%, lower than the threshold of " << pRelocatePdrThres * 100
                        << "%" << ", relocating it" << endl;
                relocateCells(neighbourId, cellPdr.first);
            }
        }
    }
}


void TschMSF::relocateCells(uint64_t neighbor, cellLocation_t cell) {
    std::vector<cellLocation_t> cellList = { cell };
    relocateCells(neighbor, cellList);
}

void TschMSF::relocateCells(uint64_t neighborId, std::vector<cellLocation_t> relocCells) {
    if (!relocCells.size()) {
        EV_DETAIL << "No cells provided to relocate" << endl;
        return;
    }

    if (pTschLinkInfo->inTransaction(neighborId)) {
        EV_WARN << "Can't relocate cells, currently in another transaction with this node" << endl;
        return;
    }

    EV_DETAIL << "Relocating cell(s) with " << MacAddress(neighborId) << " : " << relocCells << endl;

    auto availableSlots = getAvailableSlotsInRange(0, pSlotframeLength);

    if ((int) availableSlots.size() < (int) relocCells.size()) {
        EV_WARN << "Not enough free slot offsets to relocate currently scheduled cells" << endl;
        return;
    }

    std::vector<cellLocation_t> candidateCells = {};
    for (auto slof: availableSlots)
        candidateCells.push_back({slof, (offset_t) intrand(pNumChannels) });

    if (availableSlots.size() > relocCells.size() + pCellListRedundancy)
        candidateCells = pickRandomly(candidateCells, relocCells.size() + pCellListRedundancy);

    EV_DETAIL << "Selected candidate cell list to accommodate relocated cells: " << candidateCells << endl;

    for (auto cc : candidateCells)
        reservedTimeOffsets[neighborId].push_back(cc.timeOffset);

    pTsch6p->sendRelocationRequest(neighborId, MAC_LINKOPTIONS_TX, relocCells.size(), relocCells, candidateCells, pTimeout);
}

void TschMSF::handleScheduleUplink() {
    auto numDedicated = (int) pTschLinkInfo->getDedicatedCells(rplParentId).size();
    auto bundleSize = par("cellBundleSize").intValue();

    EV_DETAIL << "Handling uplink, dedicated cells found: " << numDedicated << endl;

    if ( numDedicated < 1
            || (pCellBundlingEnabled && numDedicated < bundleSize) )
    {
        bool res = addCells(rplParentId, pCellBundlingEnabled ? bundleSize : 1, MAC_LINKOPTIONS_TX);

        if (!res) {
            EV_DETAIL << "Failed to initiate ADD request, retrying in 20s" << endl;
            scheduleAt(simTime() + 20, new cMessage("", SCHEDULE_UPLINK));
        }
//        EV_DETAIL << "No dedicated TX cell found to preferred parent and "
//                << "we are currently not in transaction with it, attempting to add one TX cell" << endl;
    }
}

void TschMSF::handleScheduleDownlink(uint64_t nodeId) {
    Enter_Method_Silent();

    auto numDedicated = (int) pTschLinkInfo->getDedicatedCells(nodeId).size();
    auto numCellsToAdd = downlinkRequested[nodeId] - numDedicated;

    EV_DETAIL << "Attempting downlink scheduling to " << MacAddress(nodeId)
            << ", requested: " << downlinkRequested[nodeId] << ", scheduled: " << numDedicated << endl;

    if (numCellsToAdd < 1)
        return;

    auto res = addCells(nodeId, numCellsToAdd, MAC_LINKOPTIONS_TX);

    if (!res) {
        EV_ERROR << "Coudn't initiate 6P ADD, aborting" << endl;
        hostNode->bubble("Coudn't initiate 6P ADD, aborting");
        return;
    }

    // Something went wrong, try again
//    if (!res) {
//        EV_DETAIL << "Couldn't send 6P ADD request for some reason (check above), retrying in 20s" << endl;
//        auto msg = new cMessage("", SCHEDULE_DOWNLINK);
//        auto ci = new SfControlInfo(nodeId);
//        msg->setControlInfo(ci);
//        scheduleAt(simTime() + 20, msg);
//        return;
//    }

    if (retryInfo.find(nodeId) != retryInfo.end()) {
        std::ostringstream out;
        out << simTime() << ": trying to schedule a downlink cell to " << MacAddress(nodeId)
                << ", but the retransmission info is not empty!" << endl;

        retryInfo.erase(nodeId);
//        throw cRuntimeError(out.str().c_str());
    }

    auto ci = new SfControlInfo(nodeId);
    ci->set6pCmd(CMD_ADD);
    ci->setCellOptions(MAC_LINKOPTIONS_TX);

    retryInfo[nodeId] = ci;
}

void TschMSF::handleCellBundleReq() {
    EV_DETAIL << "Requested to schedule cell bundle, " << endl;

    if (!rplParentId)
        throw cRuntimeError("No RPL parent to schedule cell bundle with");

    if (pTschLinkInfo->inTransaction(rplParentId)) {
        EV_DETAIL << "but currently in another transaction with parent, retrying in 20s" << endl; // TODO: magic numbers
        scheduleAt(simTime() + 20, new cMessage("", CELL_BUNDLE_REQ));
        return;
    }

    auto numDedicated = (int) pTschLinkInfo->getDedicatedCells(rplParentId).size();
    if (numDedicated >= pCellBundleSize)
        EV_DETAIL << "Found " << numDedicated << " cells already scheduled, enough for a bundle" << endl;
    else
    {
        auto res = addCells(rplParentId, pCellBundleSize, MAC_LINKOPTIONS_TX);
        if (res) {
            auto ci = new SfControlInfo(rplParentId);
            ci->setNumCells(pCellBundleSize);
            ci->set6pCmd(CMD_ADD);
            ci->setCellOptions(MAC_LINKOPTIONS_TX);
            if (retryInfo.find(rplParentId) != retryInfo.end())
                throw cRuntimeError("Requested cell bundle, but retransmission info is not empty");
            retryInfo[rplParentId] = ci;
        }
        else {
            EV_WARN << "Couldn't initiate a transaction with preferred parent - " << MacAddress(rplParentId)
                    << ", see output above" << endl;
            numTranAbortedUnknownReason++;
        }
    }
}

void TschMSF::handleSelfMessage(cMessage* msg) {
    switch (msg->getKind()) {
        case REACHED_MAXNUMCELLS: {
            handleMaxCellsReached(msg);
            return;
        }
        case DO_START: {
            handleDoStart(msg);
            break;
        }
        case HOUSEKEEPING: {
            handleHousekeeping(msg);
            return;
        }
        case SCHEDULE_UPLINK: {
            handleScheduleUplink();
            break;
        }
        case CELL_BUNDLE_REQ: {
            handleCellBundleReq();
            break;
        }
        case SCHEDULE_DOWNLINK: {
            uint64_t nodeId;
            try {
                nodeId = (check_and_cast<SfControlInfo*> (msg->getControlInfo()))->getNodeId();
            }
            catch (...) {
                break;
            }

            EV << "Preparing to schedule downlink cells to " << MacAddress(nodeId) << endl;

            handleScheduleDownlink(nodeId);
            break;
        }
        case SEND_6P_REQ: {
            EV_DETAIL << "Preparing to send a 6P request" << endl;
            auto ctrlInfo = check_and_cast<SfControlInfo*> (msg->getControlInfo());

            // If this 6P request needs to be retransmitted, this check would mean
            // that the first attempt has already failed and we have to retry more
            // TODO: make separation between these two methods more clear, or just merge the two
//            if (ctrlInfo->getRtxCtn())
//                addCells(ctrlInfo);
//            else
            send6topRequest(ctrlInfo); // This just sends the request out without checking for RTX
            break;
        }
        case DEBUG_TEST: {
            handleDebugTestMsg();
            break;
        }
        case DELAY_TEST: {
            long *rankPtr = (long*) msg->getContextPointer();

 //            handleRplRankUpdate(*rankPtr, numHosts);

            auto network = hostNode->getParentModule();
            // lambda is assumed to be the same for all hosts
            handleRplRankUpdate(rplRank, network->par("numHosts"), network->par("lambda").doubleValue());

            break;
        }

        // Testing handler for cell-matching algorithm
        case CHANGE_SLOF: {
            if (!rplParentId)
                break;

            EV << "Checking unmatched rx ranges" << endl;
            auto unmatchedRanges = schedule->getUnmatchedRxRanges();
            EV << "Detected spans between RX cells not covered by a TX cell: " << endl;
            for (auto r : unmatchedRanges)
                EV << "(" << std::get<0>(r) << ", " << std::get<1>(r) << ")" << endl;
            break;
        }
        default: {
            EV_ERROR << "Unknown message received: " << msg << endl;
        }
    }
    delete msg;
}

void TschMSF::handleMessage(cMessage* msg) {
    Enter_Method_Silent();

    if (msg->isSelfMessage())
        handleSelfMessage(msg);
}

double TschMSF::getExpectedWaitingTime(int m, double pc, int rtx) {
    double lossMultiplier = 0;
    for (auto i = 1; i < rtx + 1; i++)
        lossMultiplier += (double) i * pow(pc, i);

    return getExpectedWaitingTime(m) * (1 + lossMultiplier);
}

int TschMSF::getRequiredServiceRate(double l, double pc, int rtx, int rank, int numHosts)
{

    return ceil(l * (numHosts + 2 - rank) / (1 - pc) + 0.00001);

    EV_DETAIL << "Calculating required service rate:\n" << "num hosts: " << numHosts << "\nrank: " << rank << "\nRTX: " << rtx
            << "\nETX: " << 1 / (1 - pc)
            << "\nrequired service rate: " << ceil(l * (numHosts + 2 - rank) * (double) (1 / (1 - pc) > rtx ? rtx : 1 / (1 - pc) ))
            << endl;

    if (pc <= 0 || rtx <= 0)
        return getRequiredServiceRate(l * (numHosts + 2 - rank));

    return ceil(l * (numHosts + 2 - rank) * (double) (1 / (1 - pc) > rtx ? rtx : 1 / (1 - pc) ));
//    // service rate to ensure queuing of to-be-retransmitted packets is not possible
//    if (noQueuing) {
//        auto retransmissionsRate = ceil(l * (double) (pc/(1 - pc) > rtx ? rtx : pc/(1 - pc) ));
//        EV_DETAIL << "Calculating required service rate:\n"
//                << "ETX: " << 1 / (1 - p_c)
////                << "\nrequired service rate: " << l * (double) (pc/(1 - pc) > rtx ? rtx : pc/(1 - pc))
//                << "\nrequired service rate: " << ceil(l / (1 - p_c) + 0.00001)
//                << ", ceiled: " << retransmissionsRate << endl;
//
//        return retransmissionsRate;
//    }
//
//    auto service_rate = getRequiredServiceRate(l);
//    EV_DETAIL << "Expected service rate (base): " << service_rate << endl;
//    auto wt = getExpectedWaitingTime(service_rate, pc, rtx);
//    EV_DETAIL << "Expected waiting time (base): " << getExpectedWaitingTime(service_rate) << endl;
//    EV_DETAIL << "Expected waiting time (base rtx): " << wt << endl;
//
//    while ( (wt > (double) 1/l) ) {
//        service_rate += 1;
//        wt = getExpectedWaitingTime(service_rate, pc, rtx);
//        EV_DETAIL <<  "Service rate: " << service_rate << ", expected waiting time: "
//                << wt <<  " > interarrival period (" << 1/l  << ")? " << (wt > (double) 1/l) << endl;
//    }
//
//    return service_rate;
}

void TschMSF::handleRplRankUpdate(long rank, int numHosts, double trafficRate) {
    EV_DETAIL << "Trying to schedule TX cells according to our rank - "
            << rank << " and total number of hosts - " << numHosts << endl;

    if (!rplParentId) {
        EV_WARN << "RPL parent MAC unknown, aborting" << endl;
        return;
    }

    auto currentTxCells = pTschLinkInfo->getDedicatedCells(rplParentId);
    EV_DETAIL << "Found " << currentTxCells.size() << " cells already scheduled with PP: "
            << currentTxCells << endl;

    /**
     * "numHosts - rank + 2" corresponds to the number of cells required to handle the traffic
     * from the descendant nodes (numHosts - rank) as well as the node itself (+1).
     * Additional +1 to account for the fact, that node closest to the sink are of rank 2 rather than 1.
     * 0.001 is added to account for cases lambda = mu.
     */
    auto pc = mac->par("pLinkCollision").doubleValue();
    pc = pc >= 0 ? pc : 0;
    auto rtx = mac->par("macMaxFrameRetries").intValue();

    /**
     * Calculating amount of extra traffic induced by lossy link
     */
//    double lossyFactor = 0;
//    for (auto i = 1; i < rtx + 1; i++)
//        lossyFactor += (double) i * pow(pc, i);
//    lossyFactor = (1 - pc) * lossyFactor + 1;

    if (numCellsRequired <= 0)
        numCellsRequired = getRequiredServiceRate(trafficRate, pc, rtx, rank, numHosts);

    auto numCellsLeft = numCellsRequired - (int) currentTxCells.size();

    EV_DETAIL << "Num cells required to schedule: " << numCellsLeft << endl;

    if (numCellsLeft <= 0)
        return;

    auto ctrlInfo = new SfControlInfo(rplParentId);
    ctrlInfo->set6pCmd(CMD_ADD);
    ctrlInfo->setNumCells(numCellsLeft);
    ctrlInfo->setCellOptions(MAC_LINKOPTIONS_TX);
    addCells(ctrlInfo);
}



void TschMSF::send6topRequest(SfControlInfo *ctrlInfo) {
    auto sixTopCmd = ctrlInfo->get6pCmd();
    auto nodeId = ctrlInfo->getNodeId();
    auto cellOp = ctrlInfo->getCellOptions(); // TODO: unused
    auto numCells = ctrlInfo->getNumCells();

    switch (sixTopCmd) {
        case CMD_ADD: {
            addCells(nodeId, numCells, cellOp);
            break;
        }
        case CMD_DELETE: {
            deleteCells(nodeId, numCells);
            break;
        }
        case CMD_CLEAR: {
            pTsch6p->sendClearRequest(nodeId, pTimeout);
            break;
        }
        case CMD_RELOCATE: {
            relocateCells(nodeId, ctrlInfo->getCellList());
            break;
        }
        default:
            std::string errMsg = "Cannot send out delayed 6P request, unknown 6P command type - "
                    + std::to_string(sixTopCmd);
            throw cRuntimeError(errMsg.c_str());
    }
}


void TschMSF::scheduleMinimalCells(int numMinimalCells, int slotframeLength) {
    if (numMinimalCells > slotframeLength) {
        std::ostringstream out;
        out << "More minimal cells requested (" << numMinimalCells << ") than the slotframe length (" << slotframeLength << ")" << endl;

        throw cRuntimeError(out.str().c_str());
    }

    EV_DETAIL << "Scheduling " << numMinimalCells << " minimal cells for broadcast messages: " << endl;
    auto ctrlMsg = new tsch6topCtrlMsg();
    auto macBroadcast = inet::MacAddress::BROADCAST_ADDRESS.getInt();
    ctrlMsg->setDestId(macBroadcast);
    auto cellOpts = MAC_LINKOPTIONS_TX | MAC_LINKOPTIONS_RX | MAC_LINKOPTIONS_SHARED;
    ctrlMsg->setCellOptions(cellOpts);

    std::vector<cellLocation_t> cellList = {};

    int minCellPeriod = floor(slotframeLength / numMinimalCells);
    EV_DETAIL << "Minimal cell period - " << minCellPeriod << endl;

    for (auto i = 0; i < numMinimalCells; i++) {
        auto chof = (offset_t) par("minCellChannelOffset").intValue();
        cellList.push_back({(offset_t) (i * minCellPeriod), chof > hopping->getMaxChannel() ? hopping->getNumChannels() : chof});
    }

    ctrlMsg->setNewCells(cellList);
    pTschLinkInfo->addLink(macBroadcast, false, 0, 0);
    for (auto cell : cellList)
        pTschLinkInfo->addCell(macBroadcast, cell, cellOpts);

    EV_DETAIL << cellList << endl;

    pTsch6p->updateSchedule(*ctrlMsg);
    delete ctrlMsg;
}



void TschMSF::removeAutoTxCell(uint64_t nodeId) {
    if (MacAddress(nodeId) == MacAddress::BROADCAST_ADDRESS)
        return;

    auto sharedCells = pTschLinkInfo->getSharedCellsWith(nodeId);
    if (!sharedCells.size()) {
        EV_DETAIL << "No shared cell to " << MacAddress(nodeId) << " found for deletion" << endl;
        return;
    }

    EV_DETAIL << "Removing auto cell: " << sharedCells << " of " << MacAddress(nodeId) << endl;

    pTschLinkInfo->deleteCells(nodeId, sharedCells, MAC_LINKOPTIONS_TX | MAC_LINKOPTIONS_SHARED | MAC_LINKOPTIONS_SRCAUTO);
    pTsch6p->schedule->removeAutoLinkToNeighbor(MacAddress(nodeId));

    auto cellStat = cellStatistic.find(sharedCells.back());
    if (cellStat != cellStatistic.end())
        cellStatistic.erase(cellStat);
}

tsch6pSFID_t TschMSF::getSFID() {
    Enter_Method_Silent();
    return pSFID;
}

std::vector<cellLocation_t> TschMSF::pickRandomly(std::vector<cellLocation_t> inputVec, int numRequested)
{
    if ((int) inputVec.size() < numRequested)
        return {};

    EV << "Picking randomly " << numRequested << " from vector: " << inputVec << endl;

    std::mt19937 e(intrand(1000));
    std::shuffle(inputVec.begin(), inputVec.end(), e);

    std::vector<cellLocation_t> picked = {};
    for (auto i = 0; i < numRequested; i++)
        picked.push_back(inputVec[i]);

//    std::copy(slotOffsets.begin(), slotOffsets.begin() + numSlots, picked.begin());
    return picked;
}

std::vector<cellLocation_t> TschMSF::pickConsecutively(std::vector<cellLocation_t> inputVec, int numRequested)
{
    std::vector<cellLocation_t> picked = {};
    EV << "picking consecutively from " << inputVec << endl;
    for (auto i = 0; i < numRequested && i < inputVec.size(); i++)
        picked.push_back(inputVec[i]);

    return picked;
}

bool TschMSF::slotOffsetAvailable(offset_t slOf) {
    return !pTschLinkInfo->timeOffsetScheduled(slOf)
            && slOf != autoRxCell.timeOffset
            && !slotOffsetReserved(slOf)
            && !slOfScheduled(slOf);
}

std::vector<offset_t> TschMSF::getAvailableSlotsInRange(int start, int end) {
    std::vector<offset_t> slots = {};

    if (start < 0 || end < 0) {
        EV_WARN << "Requested slot offsets in invalid range";
        return slots;
    }

    for (int slOf = start; slOf < end; slOf++)
        if (slotOffsetAvailable( (offset_t) slOf))
            slots.push_back((offset_t) slOf);

    EV_DETAIL << "\nIn range (" << start << ", " << end << ") found available slot offsets: " << slots << endl;
    return slots;
}

std::vector<offset_t> TschMSF::getAvailableSlotsInRange(int slOffsetEnd) {
    return getAvailableSlotsInRange(0, slOffsetEnd);
}

double TschMSF::getCoverageRate() {
    auto gaps = schedule->getUnmatchedRxRanges();

    std::cout << "Checking coverage rate";
    if (!gaps.size())
    {
        cout << "full cowling!" << endl;
        return 1;
    }

    vector<offset_t> slotOfs = {};

    cout << "uncovered gaps: " << endl;
    // sort out unique slot offsets
    for (auto gap : gaps)
    {
        std::cout << gap << endl;
        auto start = get<0>(gap);
        auto finish = get<1>(gap);

        if (find(slotOfs.begin(), slotOfs.end(), start) == slotOfs.end())
            slotOfs.push_back(start);

        if (find(slotOfs.begin(), slotOfs.end(), finish) == slotOfs.end())
            slotOfs.push_back(finish);
    }

    auto allRxLinks = schedule->getAllDedicatedRxLinks();

    double ratio = (allRxLinks.size() - slotOfs.size()) / (double) allRxLinks.size();

    cout << "Coverage ratio: " << ratio << endl;

    return ratio;
}

int TschMSF::createCellList(uint64_t destId, std::vector<cellLocation_t> &cellList, int numCells)
{
    Enter_Method_Silent();

    if (cellList.size()) {
        EV_WARN << "cellList should be an empty vector" << endl;
        return -EINVAL;
    }

    if (!reservedTimeOffsets[destId].empty()) {

        hostNode->bubble("There are already reserved slot offsets with this node!");
        EV_ERROR << "reservedTimeOffsets should be empty when creating new cellList,"
                << " is another transaction still in progress?\ncurrently occupied time slots: "
                << reservedTimeOffsets[destId] << endl;

        throw cRuntimeError("reservedTimeOffsets should be empty when creating new cellList");
        return -EINVAL;
    }

    std::vector<offset_t> freeSlots = {}; // generally unoccupied slot offsets
    // slot offsets from which we can construct cell list, subject to extra filtering compared to freeSlots
    std::vector<offset_t> availableSlots = {};
    std::vector<offset_t> blacklisted = {}; // slot offsets previously rejected by the receiver node

    /** Low-latency part: schedule daisy-chained cells in uplink */
    if (par("lowLatencyMode").boolValue() && destId == rplParentId)
    {
        if (uplinkSlotOffset > 0) {

            if (num6pAddSent > 0)
                numCells *= num6pAddFailed + 1; // OG: numCells *= num6pAddFailed + 1;

            // slowly increase range offered in CELL_LIST
            auto startOffset = (int) uplinkSlotOffset - numCells;
            freeSlots = getAvailableSlotsInRange(startOffset > 0 ? startOffset : 0, uplinkSlotOffset);
        }
        else
        {
            auto llStartOffset = par("lowLatencyStartingOffset").intValue();

            // 1-hop sink neighbors select slot offsets later in the slotframe to facilitate "longer" daisy-chains
            if (llStartOffset > 0)
                freeSlots = getAvailableSlotsInRange(llStartOffset, pSlotframeLength);
            else
                freeSlots = getAvailableSlotsInRange((int) pSlotframeLength / 2, pSlotframeLength);
        }
    }
    else
        freeSlots = getAvailableSlotsInRange(pSlotframeLength);

    /** Adaptive blacklisting part */
    if (blacklistedSlots.find(destId) != blacklistedSlots.end())
        blacklisted = blacklistedSlots[destId];

    if (!blacklisted.empty())
    {
        for (auto slot : freeSlots)
            if (!std::count(blacklisted.begin(), blacklisted.end(), slot))
                availableSlots.push_back(slot);

        EV_DETAIL << "Available slots after filtering blacklisted ones: " << availableSlots << endl;
    }
    else
        availableSlots = freeSlots;

    if (!availableSlots.size()) {
        EV_DETAIL << "No available cells found" << endl;
        return -ENOSPC;
    }

    // Try to schedule a TX cell after an RX one to create a checkerboard-like pattern
    // here's a weak assumption that the CELL_LIST is always being created for adding TX cells
    bool cellMatchingEnabled = par("cellMatchingEnabled").boolValue();
    if (cellMatchingEnabled)
    {
        EV << "Cell matching is enabled, checking gaps between RX cells" << endl;
        auto unmatchedRanges = schedule->getUnmatchedRxRanges();
        EV << "Detected " << unmatchedRanges.size() << " gaps between RX cells not covered by a TX cell: " << endl;
        vector<offset_t> gapSlots = {};

        for (auto gap : unmatchedRanges)
        {
            gapSlots = getAvailableSlotsInRange(get<0>(gap), get<1>(gap));

            if (gapSlots.size()) {
                EV << "Found " << gapSlots.size() << " available slots in " << gap << endl;
                break;
            }
            else {
                EV << "No free slots found in " << gap << ", going to the next one" << endl;

                // no free slots detected in the last gap, try to wrap into the next slotframe
                if (get<1>(gap) == pSlotframeLength)
                    gapSlots = getAvailableSlotsInRange(get<0>(unmatchedRanges[0]), get<1>(unmatchedRanges[0]));
            }
        }

        if (unmatchedRanges.size())
        {
            if (!gapSlots.size())
            {
                EV << "No free slot offsets found in any of the gaps?!";
                emit(uncoverableGapSignal, 1);
            }
            else
                availableSlots = gapSlots;
                EV << "Selected gap slots: " << gapSlots << endl;
        }
    }

    if ((int) availableSlots.size() <= numCells) {
        EV_DETAIL << "More or equal cells requested than available, returning "
                << availableSlots << endl;
        for (auto s : availableSlots) {
            cellList.push_back({s, (offset_t) intrand(pNumChannels)});
            reservedTimeOffsets[destId].push_back(s);
        }

        return -EFBIG;
    }

    // Fill cell list with all available slot offsets and random channel offset
    for (auto sl : availableSlots)
        cellList.push_back({sl, (offset_t) intrand(pNumChannels)});

    std::vector<cellLocation_t> temp;

    // if cell matching is enabled and it's not the first dedicated cell we're trying to add
    bool pickConsec = cellMatchingEnabled && pTschLinkInfo->getDedicatedCells(destId).size();

    // Select only required number of consecutive! cells from the cell list
    if (pCellBundlingEnabled) {
        for (auto i = 0; i < cellList.size() && i < numCells; i++)
            temp.push_back(cellList[i]);

        cellList = temp;
    }
    else
        cellList = pickConsec ? pickConsecutively(cellList, numCells) : pickRandomly(cellList, numCells);

    EV_DETAIL << "Initialized cell list: " << cellList << endl;

    // Block selected slot offsets until 6P transaction finishes
    for (auto c : cellList)
        reservedTimeOffsets[destId].push_back(c.timeOffset);

    return 0;
}

int TschMSF::pickCells(uint64_t destId, std::vector<cellLocation_t> &cellList,
                        int numCells, bool isRX, bool isTX, bool isSHARED)
{
    Enter_Method_Silent();

    if (cellList.size() == 0)
        return -EINVAL;

    EV_DETAIL << "Picking cells from list: " << cellList << endl;

    std::vector<cellLocation_t> pickedCells = {};

    for (auto cell : cellList) {
        EV_DETAIL << "proposed cell " << cell;
        if (slotOffsetAvailable(cell.timeOffset))
        {
            EV_DETAIL << " available" << endl;
            /* cell is still available, pick it. */
            pickedCells.push_back(cell);
        } else
            EV_DETAIL << " unavailable" << endl;
    }
    cellList.clear();

    // if either cell matching or cell bundling is on,
    // do not shuffle the available slot offsets, since we need to pick only consecutive ones
    if (!pCellBundlingEnabled && !par("cellMatchingEnabled")) {
        std::mt19937 e(intrand(1000));
        std::shuffle(pickedCells.begin(), pickedCells.end(), e);
    }

    for (auto i = 0; i < numCells && i < (int) pickedCells.size(); i++) {
        cellList.push_back(pickedCells[i]);
        reservedTimeOffsets[destId].push_back(pickedCells[i].timeOffset);
    }

    EV_DETAIL << "Cell list after picking: " << cellList << endl;

    if (!pickedCells.size())
        return -ENOSPC;

    if ((int) pickedCells.size() < numCells)
        return -EFBIG;

    return 0;
}

void TschMSF::clearCellStats(std::vector<cellLocation_t> cellList) {
    for (auto &cell : cellList) {
        auto cand = cellStatistic.find(cell);
        if (cand != cellStatistic.end())
            cellStatistic.erase(cand);
    }
}

bool TschMSF::checkOverlapping() {
    auto numLinks = schedule->getNumLinks();
    std::vector<offset_t> dedicatedSlOffsets = {};
    for (auto i = 0; i < numLinks; i++) {
        auto link = schedule->getLink(i);
        if (link->getAddr() != MacAddress::BROADCAST_ADDRESS && !link->isShared()) {
            auto slOf = link->getSlotOffset();
            auto nodeId = link->getAddr().getInt();
            if (std::count(dedicatedSlOffsets.begin(), dedicatedSlOffsets.end(), slOf)) {
//                handleInconsistency(link->getAddr().getInt(), 0);
                return true;
            }
            else
                dedicatedSlOffsets.push_back(slOf);
        }
    }
    return false;
}


void TschMSF::refreshDisplay() const {
    if (!rplParentId)
        return;

    std::ostringstream out;

    if (showTxCells || showTxCellCount) {
        std::vector<cellLocation_t> txCells = {};
        txCells = pTschLinkInfo->getDedicatedCells(rplParentId);


        // Paint dedicated TX cell(s) in red if overlapping cells are detected
        if (hasOverlapping)
            hostNode->getDisplayString().setTagArg("t", 2, "#fc2803");

        if (showTxCellCount)
            out << "TX: " << txCells.size();
        else {
            std::sort(txCells.begin(), txCells.end(),
                [](const cellLocation_t c1, const cellLocation_t c2) { return c1.timeOffset < c2.timeOffset; });
            out << txCells;
        }
    }

    if (showQueueUtilization) {
        std::ostringstream outUtil;
        outUtil << mac->getQueueUtilization(MacAddress(rplParentId));
        hostNode->getDisplayString().setTagArg("tt", 0, outUtil.str().c_str()); // show queue utilization above nodes
    }

    if (showQueueSize)
        out << ", Q: " << mac->getQueueSize(MacAddress(rplParentId));

    if (showLinkResets)
        out << ", R:" << numLinkResets;

    hostNode->getDisplayString().setTagArg("t", 0, out.str().c_str());

}

void TschMSF::handleSuccessAdd(uint64_t sender, int numCells, vector<cellLocation_t> cellList, vector<offset_t> reservedSlots)
{
    // No cells were added if 6P SUCCESS responds with an empty CELL_LIST, TODO: move this outside
    if (!cellList.size()) {
        EV_DETAIL << "Seems ADD to " << MacAddress(sender) << " failed" << endl;

        num6pAddFailed++;

        if (par("blacklistSlots").boolValue())
        {
            EV_DETAIL << "Slot blacklisting is active, blacklisting slots proposed in the failed transaction: " << reservedSlots << endl;
            if (blacklistedSlots.find(sender) == blacklistedSlots.end())
                blacklistedSlots[sender] = reservedSlots;
            else
                for (auto slot: reservedSlots)
                    blacklistedSlots[sender].push_back(slot);

            // TODO: Refactor into custom class for better display in Qtenv
            WATCH_VECTOR(blacklistedSlots[sender]);
        }

        if (num6pAddSent == par("trackFailed6pAddByNum").intValue())
            numFailedTracked6p++;


        // Retry scheduling low-latency cell if our rank > 2 and there are no dedicated cells at all
        if (par("lowLatencyMode").boolValue() && uplinkSlotOffset > 0 && sender == rplParentId) {

            EV_DETAIL << "Low-latency mode enabled and an empty response received from the parent" << endl;

            auto uplinkCells = pTschLinkInfo->getDedicatedCells(rplParentId);

            if ((int) uplinkCells.size() > 0) {
                EV_DETAIL << "Found uplink cells: " << uplinkCells << endl << "No retry attempt is needed" << endl;
                return;
            }

            auto selfMsg = new cMessage("", SEND_6P_REQ);

            auto ci = new SfControlInfo(sender);
            ci->set6pCmd(CMD_ADD);
            ci->setCellOptions(MAC_LINKOPTIONS_TX);

            selfMsg->setControlInfo(ci);

            auto timeout = uniform(1, 10); // originally (1, 10)

            // FIXME: magic numbers
            scheduleAt(simTime() + timeout, selfMsg);

            EV_DETAIL << "Scheduled retry attempt at " << simTime() + timeout << "s" << endl;
        }

        return;
    }

    // Check if retry is necessary
    if (retryInfo.find(sender) != retryInfo.end() && retryInfo[sender])
    {
        EV << "Found retry info stored for this neighbor" << endl;

        if (!cellList.size()) {
            EV << "No cells have been added, retrying" << endl;
            retryLastTransaction(sender, "empty response");
        }
        else
            retryInfo.erase(sender);
    }

    // if we successfully scheduled dedicated TX we don't need the shared auto cell anymore
    if (cellList.size())
        removeAutoTxCell(sender);

    // adapt the estimation window based on the up-to-date number of scheduled cells
    pMaxNumCells = ceil(par("maxNumCellsScalingFactor").doubleValue() * ((int) pTschLinkInfo->getDedicatedCells(sender).size())) + par("maxNumCells").intValue();

    // RFC limit - 254 cells as maximum size estimation window
    if (pMaxNumCells > par("maxNumTx").intValue())
        pMaxNumCells = par("maxNumTx").intValue();

    if (sender == rplParentId)
        emit(uplinkScheduledSignal, (long) cellList.back().timeOffset);

    // for delay testing mode only, need to confirm whether
    // ALL of the proposed cells were actually accepted
    if (sender == rplParentId && par("handleRankUpdates").boolValue())
        scheduleAt(simTime() + uniform(1, 3), new cMessage("", DELAY_TEST));
}

void TschMSF::handleSuccessResponse(uint64_t sender, tsch6pCmd_t cmd, int numCells, vector<cellLocation_t> cellList, vector<offset_t> reservedSlots)
{
    if (pTschLinkInfo->getLastKnownType(sender) != MSG_RESPONSE) {
        std::ostringstream out;
        out << "Handling success response to " << cmd << ", but TschLinkInfo last known message type is not MSG_RESPONSE";
        throw cRuntimeError(out.str().c_str());
    }

    switch (cmd) {
        case CMD_ADD: {
            handleSuccessAdd(sender, numCells, cellList, reservedSlots);
            break;
        }
        case CMD_DELETE: {
            clearCellStats(cellList);
            break;
        }
        case CMD_RELOCATE: {
            handleSuccessRelocate(sender, cellList);
            break;
        }
        case CMD_CLEAR: {
            clearScheduleWithNode(sender);
            scheduleAutoCell(sender);
            retryInfo.erase(sender);

            break;
        }
        default: EV_DETAIL << "Unsupported 6P command" << endl;
    }

}

void TschMSF::handleSuccessRelocate(uint64_t sender, std::vector<cellLocation_t> cellList) {
    if (!cellList.size())
        EV_WARN << "Seems RELOCATE failed, worth retrying?" << endl;
    else {
        EV_DETAIL << cellList << " are successfully relocated" << endl;
        auto cellStat = cellStatistic.find(cellList.back());

        if (cellStat != cellStatistic.end())
            cellStatistic.erase(cellStat);
    }
}

void TschMSF::clearScheduleWithNode(uint64_t neighborId)
{
    auto ctrlMsg = new tsch6topCtrlMsg();
    ctrlMsg->setDestId(neighborId);
    // FIXME: cell locations should be read directly from TschSlotframe rather than TschLinkInfo!
    std::vector<cellLocation_t> deletable = pTschLinkInfo->getCellLocations(neighborId);
    EV_DETAIL << "Clearing schedule with " << MacAddress(neighborId) << endl;

    if (deletable.size()) {
        EV_DETAIL << "Found cells to delete: " << deletable << endl;
        ctrlMsg->setDeleteCells(deletable);
        ctrlMsg->setDestId(neighborId);
        pTsch6p->updateSchedule(*ctrlMsg);
    } else
        delete ctrlMsg;

    if (nbrStatistic.find(neighborId) != nbrStatistic.end()) {
        nbrStatistic[neighborId]->numCellsElapsed = 0;
        nbrStatistic[neighborId]->numCellsUsed = 0;
        // TODO: cancel the scheduled MAX_NUM_CELLS selfmsg
//        cancelEvent(nbrStatistic[neighborId]->maxNumCellsMsg);

//        if (maxNumCellsMessages[neighborId]) {
//            cancelEvent(maxNumCellsMessages[neighborId]);
//            EV_DETAIL << "Cancelled MAX_NUM_CELLS message to " << MacAddress(neighborId);
//        }

    }
}

void TschMSF::freeReservedCellsWith(uint64_t nodeId) {
    reservedTimeOffsets[nodeId].clear();
    retryInfo.erase(nodeId);
}

/** Hacky way to free cells reserved to @param sender when link-layer ACK is received from it */
void TschMSF::handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells, std::vector<cellLocation_t> *cellList)
{
    EV_DETAIL << "Old version of handleResponse() invoked" << endl;
    reservedTimeOffsets[sender].clear();
}

void TschMSF::handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells, std::vector<cellLocation_t> cellList)
{
    Enter_Method_Silent();

    auto lastKnownCmd = pTschLinkInfo->getLastKnownCommand(sender);
    EV_DETAIL << "From " << MacAddress(sender) << " received " << code << " for "
            << lastKnownCmd << " with cell list: " << cellList << endl;

    auto reservedSlots = reservedTimeOffsets[sender];

    // free the cells that were reserved during the transaction
    reservedTimeOffsets[sender].clear();

    switch (code) {
        case RC_SUCCESS: {
            handleSuccessResponse(sender, lastKnownCmd, numCells, cellList, reservedSlots);
            break;
        }
        case RC_RESET: {
            pTsch6p->sendClearRequest(sender, pTimeout);
            retryInfo.erase(sender);
            numLinkResets++;
            break;
        }
        default: {
            numUnhandledResponses++;
            EV_WARN << "No handler specified for this command type" << endl;
        }

    }
}

void TschMSF::handleTransactionTimeout(uint64_t nodeId)
{
    hostNode->bubble("Transaction timed out");

    reservedTimeOffsets[nodeId].clear();

    if (pTschLinkInfo->getLastKnownCommand(nodeId) == CMD_CLEAR) {
        clearScheduleWithNode(nodeId);
        scheduleAutoCell(nodeId);
    }

    // FIXME: obsolete if 6P transaction timeout is set up properly
    mac->flush6pQueue(MacAddress(nodeId));
    mac->terminateTschCsmaWith(MacAddress(nodeId));

    if (retryInfo.find(nodeId) != retryInfo.end() && retryInfo[nodeId])
        retryLastTransaction(nodeId, "transaction timeout");
}

void TschMSF::retryLastTransaction(uint64_t nodeId, std::string reasonStr) {
    Enter_Method_Silent();

    if (retryInfo.find(nodeId) == retryInfo.end() || !retryInfo[nodeId]) {
        EV_WARN << "Requested to retry transaction to " << MacAddress(nodeId) << ", but no retry info found" << endl;
        return;
    }

    auto controlInfo = retryInfo[nodeId];

    EV_DETAIL << controlInfo->get6pCmd() << " to " << MacAddress(nodeId)
            << " for " << controlInfo->getNumCells() << " cells has failed due to "
            << reasonStr << ", total attempts: " << controlInfo->getRtxCtn() << endl;

    if (retryInfo[nodeId]->getRtxCtn() < par("maxRetries").intValue())
    {
        retryInfo[nodeId]->incRtxCtn();
        EV_DETAIL << "Attempting retransmit #" << retryInfo[nodeId]->getRtxCtn() << endl;
        auto msg = new cMessage("Retransmission attempt", SEND_6P_REQ);
        msg->setControlInfo(retryInfo[nodeId]);
        scheduleAt(simTime() + 0.1, msg);
    }
    else {
        EV_DETAIL << "Max retries attempted, erasing the entry from retransmissions table" << endl;
        retryInfo.erase(nodeId);
        numTranAbandonedMaxRetries++;
    }
}

void TschMSF::handle6pClearReq(uint64_t nodeId) {
    reservedTimeOffsets[nodeId].clear();
    retryInfo.erase(nodeId);
    clearScheduleWithNode(nodeId);
    scheduleAutoCell(nodeId);
}

void TschMSF::handleInconsistency(uint64_t destId, uint8_t seqNum) {
    Enter_Method_Silent();
    EV_WARN << "Inconsistency detected" << endl;
    /* Free already reserved cells to avoid race conditions */
    reservedTimeOffsets[destId].clear();
    numInconsistencies++;
    retryInfo.erase(destId);

    pTsch6p->sendClearRequest(destId, pTimeout);
}

int TschMSF::getTimeout() {
    Enter_Method_Silent();

    return pTimeout;
}

bool TschMSF::addCells(uint64_t nodeId, int numCells, uint8_t cellOptions) {
    if (numCells < 1) {
        EV_WARN << "Invalid number of cells requested - " << numCells << endl;
        return false;
    }

    bool res = false; // indication of whether 6P request has been sent

    EV_DETAIL << "Trying to add " << numCells << " cell(s) to " << MacAddress(nodeId) << endl;

    if (pTschLinkInfo->inTransaction(nodeId)) {
        EV_WARN << "Can't add cells, currently in another transaction with this node" << endl;
        return false;
    }

    std::vector<cellLocation_t> cellList = {};
    createCellList(nodeId, cellList, numCells + pCellListRedundancy);

    if (!cellList.size())
        EV_DETAIL << "No cells could be added to the cell list, aborting ADD" << endl;
    else {
        res = pTsch6p->sendAddRequest(nodeId, cellOptions, numCells, cellList, pTimeout);

        if (res)
            num6pAddSent++;
    }

    return res;
}

void TschMSF::deleteCells(uint64_t nodeId, int numCells) {
    if (numCells <= 0) {
        EV_WARN << "Invalid number of cells requested to delete" << endl;
        return;
    }

    std::vector<cellLocation_t> dedicated = pTschLinkInfo->getDedicatedCells(nodeId);

    if (!dedicated.size()) {
        EV_DETAIL << "No dedicated cells found with " << MacAddress(nodeId) << endl;

        auto pktsEnqueued = mac->getQueueSize(MacAddress(nodeId));

        EV_DETAIL << pktsEnqueued << " packets enqueued" << endl;

        if (!pktsEnqueued && par("allowAutoCellDeletion").boolValue())
            removeAutoTxCell(nodeId);

        return;
    }

    if (((int) dedicated.size()) - numCells < 1) {
        EV_WARN << "Cannot delete last dedicated cell" << endl;
        return;
    }

    std::vector<cellLocation_t> cellList;

    if (numCells == 1)
        cellList.push_back(dedicated.back());
    else
        cellList = pickRandomly(dedicated, numCells);

    EV_DETAIL  << "Chosen for deletion: " << cellList << endl;
    pTsch6p->sendDeleteRequest(nodeId, MAC_LINKOPTIONS_TX, numCells, cellList, pTimeout);
}

bool TschMSF::slotOffsetReserved(offset_t slOf) {
    std::map<uint64_t, std::vector<offset_t>>::iterator nodes;

    for (nodes = reservedTimeOffsets.begin(); nodes != reservedTimeOffsets.end(); ++nodes) {
        if (slotOffsetReserved(nodes->first, slOf))
            return true;
    }

    return false;
}

bool TschMSF::slotOffsetReserved(uint64_t nodeId, offset_t slOf) {
    std::vector<offset_t> slOffsets = reservedTimeOffsets[nodeId];
    return std::find(slOffsets.begin(), slOffsets.end(), slOf) != slOffsets.end();
}

void TschMSF::handleParentChangedSignal(uint64_t newParentId) {
    EV_DETAIL << "RPL parent changed to " << MacAddress(newParentId) << endl;

    // If RPL parent hasn't been set, we've just joined the DODAG
    if (!rplParentId) {
        rplParentId = newParentId;

        // To control whether secondary nodes are allowed to occupy the cells (mainly an issue at the sink)
        if (par("scheduleUplinkOnJoin").boolValue()) {
            int numCellsToSchedule = pCellBundlingEnabled ? par("cellBundleSize").intValue() : par("initialNumCells").intValue();
            addCells(rplParentId, numCellsToSchedule, MAC_LINKOPTIONS_TX);
        }

        // FIXME: magic numbers
        if (par("scheduleUplinkManually").boolValue())
            scheduleAt(simTime() + 20, new cMessage("", SCHEDULE_UPLINK));

        return;
    }

    auto txCells = pTschLinkInfo->getDedicatedCells(rplParentId);
    EV_DETAIL << "Dedicated TX cells currently scheduled with PP: " << txCells << endl;

    /** Clear all negotiated cells with the former parent */
    pTsch6p->sendClearRequest(rplParentId, pTimeout);

    rplParentId = newParentId;

    /** and schedule the same amount of cells (or at least 1) with the new parent */
    if (par("scheduleUplinkOnJoin").boolValue())
        addCells(newParentId, (int) txCells.size() > 0 ? (int) txCells.size() : par("initialNumCells").intValue(), MAC_LINKOPTIONS_TX);
}

void TschMSF::handlePacketEnqueued(uint64_t dest) {
    if (MacAddress(dest) == MacAddress::BROADCAST_ADDRESS)
        return;

    auto txCells = pTschLinkInfo->getCellsByType(dest, MAC_LINKOPTIONS_TX);

    EV_DETAIL << "Received MAC notification for a packet addressed to "
            << MacAddress(dest) << ", TX cells: " << txCells << endl;

    if (pCheckScheduleConsistency)
        checkScheduleConsistency(dest);

    // Heuristic, checking if there's a dedicated cell to preferred parent whenever a packet is enqueued
    if (rplParentId == dest || par("downlinkDedicated").boolValue())
    {
        // EXPERIMENTAL: forbid leaf nodes (seatbelts) at the sink to schedule uplink
        if (!par("scheduleUplinkOnJoin").boolValue() && isLeafNode && rplRank == 2)
            return;

        auto dedicatedCells = pTschLinkInfo->getDedicatedCells(dest);

        if (!dedicatedCells.size() && !pTschLinkInfo->inTransaction(dest)) {
            EV_DETAIL << "No dedicated TX cell found to this node, and "
                    << "we are currently not in transaction with it, attempting to add one TX cell" << endl;
            addCells(dest, 1, MAC_LINKOPTIONS_TX);
        }
    }

    // Ensure minimal connectivity
    if (!txCells.size())
        scheduleAutoCell(dest);
}

void TschMSF::checkScheduleConsistency(uint64_t nodeId) {
    auto txCells = pTschLinkInfo->getCellsByType(nodeId, MAC_LINKOPTIONS_TX);

    for (auto cell : txCells) {
        if (!schedule->getLinkByCellCoordinates(cell.timeOffset, cell.channelOffset, MacAddress(nodeId))) {
            std::ostringstream ostream;
            ostream << "Schedule inconsistency detected on node " << MacAddress(nodeId)
                    << ": TX cell at [ " << cell.timeOffset << ", "
                    << cell.channelOffset << " ] is present in TschLinkInfo but missing from the schedule!" << endl;

            throw cRuntimeError(ostream.str().c_str());
        }
    }
}

void TschMSF::receiveSignal(cComponent *src, simsignal_t id, cObject *value, cObject *details)
{
    Enter_Method_Silent();

    std::string signalName = getSignalName(id);

    static std::vector<std::string> namesNeigh = {}; //{"nbSlot-neigh-", "nbTxFrames-neigh-", "nbRxFrames-neigh-"};
    static std::vector<std::string> namesLink = {"nbSlot-link-", "nbTxFrames-link-", "nbRecvdAcks-link-"};

    // this is a hint we should check for new dynamic signals
    if (dynamic_cast<cPostParameterChangeNotification *>(value)) {
        // consider all neighbors we already know
        for (auto const& neigbhor : neighbors) {
            // subscribe to all neighbor-related signals
            auto macStr = MacAddress(neigbhor).str();
            for (auto & name: namesNeigh) {
                auto fullname = name + macStr;
                if (src->isSubscribed(fullname.c_str(), this) == false)
                    src->subscribe(fullname.c_str(), this);
            }

            // subscribe to all link related signals
            for (auto & cell :pTschLinkInfo->getCells(neigbhor)) {
                auto slotOffset = std::get<0>(cell).timeOffset;
                auto channelOffset = std::get<0>(cell).channelOffset;

                for (auto & name: namesLink) {
                    std::stringstream fullname;
                    fullname << name << "0." << slotOffset << "." << channelOffset;
                    if (src->isSubscribed(fullname.str().c_str(), this) == false)
                        src->subscribe(fullname.str().c_str(), this);
                }
            }
        }
    }

    if (id == linkBrokenSignal && par("lowLatencyMode").boolValue() && uplinkSlotOffset > 0)
    {
        Packet *datagram = check_and_cast<Packet *>(value);
        std::string packetName(datagram->getFullName());
        EV_DETAIL << "MSF received link break for " << packetName << endl;

        // FIXME: checks also if the lost 6P message was really addressed to the parent
        if (packetName.find("6top") != std::string::npos && packetName.find("Req") != std::string::npos) {
            auto uplinkCells = pTschLinkInfo->getDedicatedCells(rplParentId);

            if ((int) uplinkCells.size() > 0)
                return;

            EV_DETAIL << "Seems 6P packet has been lost and there are no uplink cells to pref. parent, let's retry scheduling uplink cell" << endl;

            auto selfMsg = new cMessage("", SEND_6P_REQ);

            auto ci = new SfControlInfo(rplParentId);
            ci->set6pCmd(CMD_ADD);
            ci->setCellOptions(MAC_LINKOPTIONS_TX);

            selfMsg->setControlInfo(ci);

            // FIXME: magic numbers
            scheduleAt(simTime() + uniform(1, 10), selfMsg);
        }
    }
}

void TschMSF::receiveSignal(cComponent *src, simsignal_t id, long value, cObject *details)
{
    Enter_Method_Silent();

    if (!hasStarted)
        return;

    std::string signalName = getSignalName(id);

    if (std::strcmp(signalName.c_str(), "parentChanged") == 0) {
        auto rplControlInfo = (RplGenericControlInfo*) details;
        handleParentChangedSignal(rplControlInfo->getNodeId());
        return;
    }

    if (std::strcmp(signalName.c_str(), "tschScheduleDownlink") == 0) {

        // this would work for IP addresses of advertised destinations,
        // but the MAC we get in this signal is always only the "last hop"
//        if (std::count(downlinkRequested.begin(), downlinkRequested.end(), value))
//            return;

        pLimNumCellsUsedLow = -1; // to avoid deletion of specially-provisioned cells

        if (downlinkRequested.find(value) == downlinkRequested.end())
            downlinkRequested[value] = 0;

        downlinkRequested[value]++;

        // FIXME: spaghetti, need to track that a cell has ALREADY been requested for specific destination
        if (downlinkRequested[value] > 4)
            return;

        EV_DETAIL << "Downlink requested to child " << MacAddress(value) << endl;
        auto msg = new cMessage("Schedule downlink", SCHEDULE_DOWNLINK);
        auto ci = new SfControlInfo(value);
        msg->setControlInfo(ci);
        // Wait some time before sending ADD req, since the child is most likely
        // trying to schedule an uplink cell with us at the same time
        scheduleAt(simTime() + 100, msg); // TODO: use reasonable timeout

        return;
    }

    // whenever a DAO has been forwarded, that requests a cell bundle in the uplink
    if (std::strcmp(signalName.c_str(), "tschScheduleUplink") == 0 && par("handleCellBundlingSignal").boolValue()) {
        // Avoid duplicate requests
        if (pCellBundlingEnabled)
            return;

        pCellBundlingEnabled = true;
        pLimNumCellsUsedLow = -1; // to avoid deletion of bundled cells
        handleCellBundleReq();
        return;
    }

    if (id == mac->pktRecFromUpperSignal) {
        auto macCtrlInfo = (Ieee802154eMac::MacGenericInfo*) details;
        handlePacketEnqueued(macCtrlInfo->getNodeId());
        return;
    }

    if (id == mac->pktRecFromLowerSignal) {
        if (find(neighbors.begin(), neighbors.end(), value) == neighbors.end())
        {
            neighbors.push_back(value);
            EV << "Added new neighbor: " << MacAddress(value) << endl;
        }

        return;
    }

    if (std::strcmp(signalName.c_str(), "rankUpdated") == 0) {
        rplRank = (int) value;

        // Custom delay testing handler procedure
        if (par("handleRankUpdates").boolValue())
        {
            auto selfMsg = new cMessage("", DELAY_TEST);
//            selfMsg->setContextPointer((long*) &value); TODO: doesn't work, fix
            // Try to add a few extra cells as required by the expected traffic rate
            scheduleAt(simTime() + SimTime(20, SIMTIME_S), selfMsg);
        }

        // Custom ReSA demo use case handling procedure
        // For seatbelts which are not direct neighbors of the sink, schedule dedicated cell with the parent
        // to avoid blocking his auto RX cell when the applications start
        if (!par("scheduleUplinkOnJoin").boolValue() && rplRank > 2) {
            auto dedicatedCells = pTschLinkInfo->getDedicatedCells(rplParentId);

            if (!dedicatedCells.size() && !pTschLinkInfo->inTransaction(rplParentId))
                addCells(rplParentId, 1, MAC_LINKOPTIONS_TX);
        }

        return;
    }

    if (std::strcmp(signalName.c_str(), "childJoined") == 0) {
        isLeafNode = false; // TODO: Add reset mechanism
        return;
    }

    // As part of low-latency scheduling, RPL notifies the SF about the lowest
    // time offset used by the preferred parent for his own uplink
    // TODO: rename signal for clarity
    if (par("lowLatencyMode").boolValue() && std::strcmp(signalName.c_str(), "uplinkSlotOffset") == 0) {
        uplinkSlotOffset = (uint8_t) value;
        return;
    }

    std::string statisticStr;
    std::string scopeStr;

    auto pos = signalName.find('-'), pos2 = pos;

    if (pos != std::string::npos) {
        statisticStr = signalName.substr(0, pos);

        pos2 = signalName.find('-', pos + 1);
        if (pos2 != std::string::npos)
            scopeStr = signalName.substr(pos+1, pos2-pos-1);
        else
            return; //error
    } else
        return; //error

    if (scopeStr != "link")
        return;

    std::string linkStr = signalName.substr(pos2+1, signalName.size());
    pos = linkStr.find('.');
    pos2 = linkStr.find('.', pos + 1);
    offset_t slotOffset = std::stoi(linkStr.substr(pos + 1, pos2-pos-1));
    offset_t channelOffset = std::stoi(linkStr.substr(pos2 + 1, linkStr.size()-pos-1));
    cellLocation_t cell = {slotOffset, channelOffset};

    auto neighbor = pTschLinkInfo->getNodeOfCell(cell);
    auto options = pTschLinkInfo->getCellOptions(neighbor, cell);

    if (neighbor == 0) {
        std::ostringstream out;

        out << "could not find neighbor for cell " << linkStr << endl;
        throw cRuntimeError(out.str().c_str());
    }

    if (options != 0xFF && getCellOptions_isTX(options) && neighbor != MacAddress::BROADCAST_ADDRESS.getInt())
    {
        updateNeighborStats(neighbor, statisticStr);
        updateCellTxStats(cell, statisticStr);
    }

}

void TschMSF::updateCellTxStats(cellLocation_t cell, std::string statType) {
    if (cellStatistic.find(cell) == cellStatistic.end())
        cellStatistic.insert({cell, {0, 0, 0}});

    if (statType == "nbTxFrames") {
        cellStatistic[cell].NumTx++;
        if (cellStatistic[cell].NumTx >= pMaxNumTx) {
            cellStatistic[cell].NumTx =  cellStatistic[cell].NumTx / 2;
            cellStatistic[cell].NumTxAck = cellStatistic[cell].NumTxAck / 2;
        }
    } else if (statType == "nbRecvdAcks")
        cellStatistic[cell].NumTxAck++;
}

void TschMSF::incrementNeighborCellElapsed(uint64_t neighborId) {
    Enter_Method_Silent();
    if (nbrStatistic.find(neighborId) == nbrStatistic.end())
        nbrStatistic.insert({neighborId, new NbrStatistic(neighborId)});

    nbrStatistic[neighborId]->numCellsElapsed++;
    checkMaxCellsReachedFor(neighborId);
}

void TschMSF::decrementNeighborCellElapsed(uint64_t neighborId) {
    if (nbrStatistic.find(neighborId) == nbrStatistic.end()) {
        nbrStatistic.insert({neighborId, new NbrStatistic(neighborId)});
        return;
    }

    nbrStatistic[neighborId]->numCellsElapsed--;
}

void TschMSF::checkMaxCellsReachedFor(uint64_t neighborId) {
    Enter_Method_Silent();

    if (nbrStatistic[neighborId]->numCellsElapsed >= pMaxNumCells)
    {
        EV_DETAIL << "Reached MAX_NUM_CELLS with " << MacAddress(neighborId) << endl;
        if (maxNumCellsMessages.find(neighborId) == maxNumCellsMessages.end()) {
            auto maxNumCellMsg = new cMessage("MAX_NUM_CELLS", REACHED_MAXNUMCELLS);
            maxNumCellMsg->setContextPointer(new MacAddress(neighborId));
            maxNumCellsMessages[neighborId] = maxNumCellMsg;
        }

        if (maxNumCellsMessages[neighborId] && maxNumCellsMessages[neighborId]->isScheduled()) {
            EV_DETAIL << "MAX_NUM_CELLS message already scheduled, aborting and re-scheduling a new one" << endl;
            cancelEvent(maxNumCellsMessages[neighborId]);
        }

        scheduleAt(simTime(), maxNumCellsMessages[neighborId]);
    }
}

void TschMSF::updateNeighborStats(uint64_t neighborId, std::string statType) {
    if (nbrStatistic.find(neighborId) == nbrStatistic.end())
        nbrStatistic.insert({neighborId, new NbrStatistic(neighborId)});

    if (statType == "nbSlot") {
        nbrStatistic[neighborId]->numCellsElapsed++;
        EV_DETAIL << "NumCellsElapsed for " << MacAddress(neighborId) << " now at " << +(nbrStatistic[neighborId]->numCellsElapsed) << endl;
    } else if (statType == "nbTxFrames") {
        nbrStatistic[neighborId]->numCellsUsed++;

        // If stats are reset while there's a transmission, the elapsed counter is 0, while used is incremented to 1
        if (!nbrStatistic[neighborId]->numCellsElapsed)
            nbrStatistic[neighborId]->numCellsElapsed = nbrStatistic[neighborId]->numCellsUsed;

        EV_DETAIL << "NumCellsUsed for " << MacAddress(neighborId) << " now at " << +(nbrStatistic[neighborId]->numCellsUsed) << endl;
    }

    checkMaxCellsReachedFor(neighborId);
}

uint32_t TschMSF::saxHash(int maxReturnVal, InterfaceToken EUI64addr)
{
    uint32_t l_bit = 1, r_bit = 1; // TODO choose wisely

    uint64_t EUI64addr64 = (((uint64_t) EUI64addr.normal()) << 32) | EUI64addr.low();
    auto c = static_cast<char*>(static_cast<void*>(&EUI64addr64));

    // step 1
    uint32_t i = 0; // byte index of EUI64addr
    uint32_t h = 0; // intermediate hash value = h0

    do {
        // step 2
        uint32_t sum = (h << l_bit) + (h >> r_bit) + ((uint8_t) c[i]);
        h = (sum ^ h) % maxReturnVal;
        i++;
    } while (i < sizeof(uint64_t));

    return h;
}

void TschMSF::handleDebugTestMsg() {
    if (!rplParentId)
        return;

    if (simTime() >= 200) {
        EV_DETAIL << "SimTime > 100, adding uplink cell" << endl;

        hostNode->bubble("Sending ADD");

        addCells(rplParentId, 1, MAC_LINKOPTIONS_TX);
        return;
    }

    if (simTime() >= 50) {
        EV_DETAIL << "Debug self-msg received, preparing to send 6P CLEAR to our pref.parent - "
                << MacAddress(rplParentId) << endl;
//                hostNode->bubble("Sending CLEAR");
//                pTsch6p->sendClearRequest(rplParentId, pTimeout);
        hostNode->bubble("Sending ADD");
        addCells(rplParentId, 1, MAC_LINKOPTIONS_TX);

        return;
    }
}
