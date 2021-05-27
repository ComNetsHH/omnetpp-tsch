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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "TschMSF.h"

#include "../Ieee802154eMac.h"
#include "Rpl.h"
#include "Tsch6tischComponents.h"
#include <omnetpp.h>
#include <random>
#include <algorithm>

using namespace tsch;

Define_Module(TschMSF);

TschMSF::TschMSF() :
    totalElapsed(0),
    rplParentId(0),
    tsch6pRtxThresh(3),
    numHosts(0),
    hasStarted(false),
    pHousekeepingDisabled(false),
    hasOverlapping(false)
{
}
TschMSF::~TschMSF() {
}

void TschMSF::initialize(int stage) {
    if (stage == 0) {
        EV_DETAIL << "MSF initializing" << endl;
        pNumChannels = getParentModule()->par("nbRadioChannels").intValue();
        pTschLinkInfo = (TschLinkInfo*) getParentModule()->getSubmodule("linkinfo");
        pTimeout = par("timeout").intValue();
        pMaxNumCells = par("maxNumCells");
        pMaxNumTx = par("MAX_NUMTX");
        pLimNumCellsUsedHigh = par("upperCellUsageLimit");
        pLimNumCellsUsedLow = par("lowerCellUsageLimit");
        pRelocatePdrThres = par("RELOCATE_PDRTHRES");
        pCellListRedundancy = par("cellListRedundancy").intValue();
        pNumMinimalCells = par("numMinCells").intValue();
        disable = par("disable").boolValue();
        pHousekeepingPeriod = par("housekeepingPeriod").intValue();
        pHousekeepingDisabled = par("disableHousekeeping").boolValue();
        internalEvent = new cMessage("SF internal event", UNDEFINED);
        s_InitialScheduleComplete = registerSignal("initial_schedule_complete");
        rplParentChangedSignal = registerSignal("rplParentChanged");
        pAutoCellOnDemand = par("autoCellOnDemand").boolValue();
    } else if (stage == 5) {
        interfaceModule = dynamic_cast<InterfaceTable *>(getParentModule()->getParentModule()->getParentModule()->getParentModule()->getSubmodule("interfaceTable", 0));
        pNodeId = interfaceModule->getInterface(1)->getMacAddress().getInt();
        pSlotframeLength = getModuleByPath("^.^.schedule")->par("macSlotframeSize").intValue();
        pTsch6p = (Tsch6topSublayer*) getParentModule()->getSubmodule("sixtop");
        mac = check_and_cast<Ieee802154eMac*>(getModuleByPath("^.^.mac"));
        mac->subscribe(POST_MODEL_CHANGE, this);
        mac->subscribe("pktEnqueued", this);

        schedule = check_and_cast<TschSlotframe*>(getModuleByPath("^.^.schedule"));

        if (disable)
            return;

        hostNode = getModuleByPath("^.^.^.^.");
        numHosts = hostNode->getParentModule()->par("numHosts").intValue();

        EV_DETAIL << "Found a total of " << numHosts << " in the network" << endl;

        showTxCells = par("showDedicatedTxCells").boolValue();
        showQueueUtilization = par("showQueueUtilization").boolValue();
        showTxCellCount = par("showTxCellCount").boolValue();

        /** Schedule minimal cells for broadcast control traffic [RFC8180, 4.1] */
        scheduleMinimalCells();
        /** And an auto RX cell for communication with neighbors [IETF MSF draft, 3] */
        scheduleAutoRxCell(interfaceModule->getInterface(1)->getMacAddress().formInterfaceIdentifier());

        rpl = hostNode->getSubmodule("rpl");
        if (!rpl) {
            EV_WARN << "RPL module not found" << endl;
            return;
        }

        rpl->subscribe("parentChanged", this);
        rpl->subscribe("rankUpdated", this);
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

    autoRxCell = {euiAddr.low() % pSlotframeLength, euiAddr.low() % pNumChannels}; // heuristics
    cellList.push_back(autoRxCell);
    ctrlMsg->setNewCells(cellList);
    EV_DETAIL << "Scheduling auto RX cell at " << cellList << endl;

    // Check if auto RX overlaps with minimal cell
    auto overlappingMinCells = pTschLinkInfo->getMinimalCells(autoRxCell.timeOffset);
    if (overlappingMinCells.size()) {
        EV_DETAIL << "Auto RX cell conflicts with minimal cell at " << autoRxCell.timeOffset
                << " slotOffset, deleting this minimal cell to avoid scheduling issues" << endl;
        ctrlMsg->setDeleteCells(overlappingMinCells);
    }

    pTsch6p->updateSchedule(*ctrlMsg);
    initialScheduleComplete[pNodeId] = true;
    delete ctrlMsg;
}

void TschMSF::scheduleAutoCell(uint64_t neighbor) {
    if (pTschLinkInfo->sharedTxScheduled(neighbor))
        return;

    auto ctrlMsg = new tsch6topCtrlMsg();
    ctrlMsg->setDestId(neighbor);
    ctrlMsg->setCellOptions(MAC_LINKOPTIONS_TX | MAC_LINKOPTIONS_SHARED | MAC_LINKOPTIONS_SRCAUTO);

    auto neighborIntfIdent = MacAddress(neighbor).formInterfaceIdentifier();

    uint32_t nbruid = neighborIntfIdent.low();

//    EV_DETAIL << "Neighbor " << MacAddress(neighbor) << " low - " << std::to_string(nbruid)
//        << ", possible auto cell at " << std::to_string(nbruid % pSlotframeLen) << ", " << std::to_string(nbruid % pNumChannels) << endl;

    std::vector<cellLocation_t> cellList;
//    cellList.push_back({1 + saxHash(pSlotframeLen - 1, neighborIntfIdent), saxHash(16, neighborIntfIdent)});

    auto slotOffset = nbruid % pSlotframeLength;
    cellList.push_back({slotOffset, nbruid % pNumChannels});
    ctrlMsg->setNewCells(cellList);

    EV_DETAIL << "Scheduling auto TX cell at " << cellList << " to neighbor - " << MacAddress(neighbor) << endl;

    pTschLinkInfo->addLink(neighbor, false, 0, 0);
    pTschLinkInfo->addCell(neighbor, cellList.back(), MAC_LINKOPTIONS_TX | MAC_LINKOPTIONS_SHARED | MAC_LINKOPTIONS_SRCAUTO);
    pTsch6p->updateSchedule(*ctrlMsg);
    delete ctrlMsg;
}

void TschMSF::finish() {
    for (auto const& entry : nbrStatistic)
        std::cout << MacAddress(entry.first).str() << " elapsed "
            << +(entry.second.NumCellsElapsed) << " used " << +(entry.second.NumCellsUsed) << endl;
}

void TschMSF::start() {
    Enter_Method_Silent();

    if (disable)
        return;

    tsch6topCtrlMsg* msg = new tsch6topCtrlMsg();
    msg->setKind(DO_START);

    scheduleAt(simTime() + SimTime(par("startTime").doubleValue()), msg);
}

std::string TschMSF::printCellUsage(std::string neighborMac, double usage) {
    std::string usageInfo = std::string(" Usage to/from neighbor ") + neighborMac
            + std::string(usage >= pLimNumCellsUsedHigh ? " exceeds upper" : "")
            + std::string(usage <= pLimNumCellsUsedLow ? " below lower" : "")
            + std::string((usage < pLimNumCellsUsedHigh && usage > pLimNumCellsUsedLow) ? " within" : "")
            + std::string(" limit with ") + std::to_string(usage);

    return usageInfo;
//    std::cout << pNodeId << usageInfo << endl;
}

void TschMSF::checkDedicatedCellScheduled(uint64_t neighbor) {
    if (!pTschLinkInfo->getDedicatedCells(neighbor).size() && !pTschLinkInfo->inTransaction(neighbor))
        addCells(neighbor, 1);
}

bool TschMSF::slOfScheduled(offset_t slOf) {
    auto links = schedule->getLinks();
    for (auto l : links)
        if (slOf == l->getSlotOffset())
            return true;
    return false;
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
                handleInconsistency(link->getAddr().getInt(), 0);
                return true;
            }
            else
                dedicatedSlOffsets.push_back(slOf);
        }
    }
    return false;
}

void TschMSF::handleMaxCellsReached(cMessage* msg) {
    auto neighborMac = (MacAddress*) msg->getContextPointer();
    auto neighborId = neighborMac->getInt();

    // we might get a notification from our own mac
    if (neighborId == pNodeId)
        return;

    EV_DETAIL << "MAX_NUM_CELLS reached, assessing cell usage with " << *neighborMac << ":"
            << endl << "Currently scheduled cells: \n" << pTschLinkInfo->getCells(neighborId) << endl;

    double usage = (double) nbrStatistic[neighborId].NumCellsUsed / nbrStatistic[neighborId].NumCellsElapsed;

    EV_DETAIL << printCellUsage(neighborMac->str(), usage) << endl;

    if (usage >= pLimNumCellsUsedHigh)
        addCells(neighborId, par("cellBandwidthIncrement").intValue());
    if (usage <= pLimNumCellsUsedLow)
        deleteCells(neighborId, 1);

    // reset values
    nbrStatistic[neighborId].NumCellsUsed = 0; //intrand(pMaxNumCells >> 1);
    nbrStatistic[neighborId].NumCellsElapsed = 0;

    delete neighborMac;
}

void TschMSF::handleDoStart(cMessage* msg) {
    hasStarted = true;
    EV_DETAIL << "MSF has started" << endl;
//
//    if (!pHousekeepingDisabled)
//        scheduleAt(simTime() + SimTime(par("housekeepingStart"), SIMTIME_S), new tsch6topCtrlMsg("", HOUSEKEEPING));

    /** Get all nodes that are within communication range of @p nodeId.
    *   Note that this only works if all nodes have been initialized (i.e.
    *   maybe not in init() step 1!) **/
    neighbors = pTsch6p->getNeighborsInRange(pNodeId, mac);

    // By MSF draft we shouldn't really schedule an auto cell to each neighbor at startup,
    // keeping this as an option for compatibility reasons
    for (auto & neighbor : neighbors) {
        if (!pAutoCellOnDemand)
            scheduleAutoCell(neighbor);
        initialScheduleComplete[neighbor] = true;
    }
}

void TschMSF::handleHousekeeping(cMessage* msg) {
    EV_DETAIL << "Performing housekeeping: " << endl;
    scheduleAt(simTime()+ uniform(0.5, 1.25) * SimTime(pHousekeepingPeriod, SIMTIME_S), msg);

    // Try to ensure there's a dedicated uplink cell through preferred parent,
    // as well as dedicated downlink cell per neighbor
    if (rplParentId) {
        // OPTIONAL: periodic cell overlap detection mechanism
        if (par("cellOverlapDetection").boolValue()) {
            totalElapsed++;
            // FIXME: magic numbers
            if (totalElapsed > 5) {
                hasOverlapping = checkOverlapping();
                totalElapsed = 0;
            }
        }

        checkDedicatedCellScheduled(rplParentId);
    }

    if (par("keepDedicatedDownlink").boolValue()) {
        // OPTIONAL: keep a dedicated downlink cell
        for (auto oneHopChild : oneHopRplChildren)
            checkDedicatedCellScheduled(oneHopChild);
    }

    return;

    // iterate over all neighbors
    for (auto const& entry : initialScheduleComplete) {
        std::map<cellLocation_t, double> pdrStat;

        // calc cell PDR per neighbor
        for (auto & cell :pTschLinkInfo->getCells(entry.first)) {
            auto slotOffset = std::get<0>(cell).timeOffset;
            auto channelOffset = std::get<0>(cell).channelOffset;
            cellLocation_t cellLoc = {slotOffset, channelOffset};

            auto it = cellStatistic.find(cellLoc);
            if (it == cellStatistic.end())
                continue;

            double cellPdr = static_cast<double>(std::get<1>(*it).NumTxAck)
                    / static_cast<double>(std::get<1>(*it).NumTx);
            pdrStat.insert({cellLoc, cellPdr});
//            EV_DETAIL << "Calculated cell " << cellLoc << " pdr = " << cellPdr << endl;
//            EV_DETAIL << "PDR stat:c " << pdrStat << endl;
        }

        if (pdrStat.size() <= 1)
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

            if ((maxPdr->second - cellPdr.second) > pRelocatePdrThres)
                relocateCells(entry.first, cellPdr.first);
        }
    }
}

void TschMSF::relocateCells(uint64_t neighbor) {
    relocateCells(neighbor, pTschLinkInfo->getDedicatedCells(neighbor));
}

void TschMSF::relocateCells(uint64_t neighbor, cellLocation_t cell) {
    std::vector<cellLocation_t> cellList = { cell };
    relocateCells(neighbor, cellList);
}

void TschMSF::relocateCells(uint64_t neighbor, std::vector<cellLocation_t> relocCells) {
    if (!relocCells.size()) {
        EV_DETAIL << "No cells found to relocate" << endl;
        return;
    }
    EV_DETAIL << "Relocating cell(s) with " << MacAddress(neighbor) << " : " << relocCells << endl;

    auto availableSlots = getAvailableSlotsInRange(pSlotframeLength);

    if (availableSlots.size() < relocCells.size()) {
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
        reservedTimeOffsets[neighbor].push_back(cc.timeOffset);

    pTsch6p->sendRelocationRequest(neighbor, MAC_LINKOPTIONS_TX, relocCells.size(), relocCells, candidateCells, pTimeout);
}

void TschMSF::handleMessage(cMessage* msg) {
    Enter_Method_Silent();

    if (!msg->isSelfMessage()) {
        delete msg;
        return;
    }

    switch (msg->getKind()) {
        case REACHED_MAXNUMCELLS: {
            handleMaxCellsReached(msg);
            break;
        }
        case DO_START: {
            handleDoStart(msg);
            break;
        }
        case HOUSEKEEPING: {
            handleHousekeeping(msg);
            return;
        }
        case DELAY_TEST: {
            long *rankPtr = (long*) msg->getContextPointer();
            EV_DETAIL << "Received DELAY_TEST self-msg, rank - " << *rankPtr << endl;

//            handleRplRankUpdate(*rankPtr, numHosts);

            handleRplRankUpdate(rplRank, numHosts);

            break;
        }
        default: EV_ERROR << "Unknown message received: " << msg << endl;
    }
    delete msg;
}

void TschMSF::scheduleMinimalCells() {
    EV_DETAIL << "Scheduling minimal cells for broadcast messages: " << endl;
    auto ctrlMsg = new tsch6topCtrlMsg();
    auto macBroadcast = inet::MacAddress::BROADCAST_ADDRESS.getInt();
    ctrlMsg->setDestId(macBroadcast);
    auto cellOpts = MAC_LINKOPTIONS_TX | MAC_LINKOPTIONS_RX | MAC_LINKOPTIONS_SHARED;
    ctrlMsg->setCellOptions(cellOpts);

    std::vector<cellLocation_t> cellList = {};

    int minCellPeriod = floor(pSlotframeLength / pNumMinimalCells);
    EV_DETAIL << "Min cells period - " << minCellPeriod << endl;

    for (offset_t i = 0; i < pNumMinimalCells; i++)
        cellList.push_back({i * minCellPeriod, 0});

    ctrlMsg->setNewCells(cellList);
    pTschLinkInfo->addLink(macBroadcast, false, 0, 0);
    for (auto cell : cellList)
        pTschLinkInfo->addCell(macBroadcast, cell, cellOpts);

    EV_DETAIL << cellList << endl;

    pTsch6p->updateSchedule(*ctrlMsg);
    delete ctrlMsg;
}


void TschMSF::removeAutoTxCell(uint64_t neighbor) {
    std::vector<cellLocation_t> cellList;

    for (auto &cell: pTschLinkInfo->getCells(neighbor))
        if (std::get<1>(cell) != 0xFF && getCellOptions_isAUTO(std::get<1>(cell)))
            cellList.push_back(std::get<0>(cell));

    if (cellList.size()) {
        EV_DETAIL << "Removing auto TX cell " << cellList << " to neighbor " << MacAddress(neighbor) << endl;
        auto ctrlMsg = new tsch6topCtrlMsg();
        ctrlMsg->setDestId(neighbor);
        pTschLinkInfo->deleteCells(neighbor, cellList, MAC_LINKOPTIONS_TX | MAC_LINKOPTIONS_SHARED | MAC_LINKOPTIONS_SRCAUTO);
        ctrlMsg->setDeleteCells(cellList);
        pTsch6p->updateSchedule(*ctrlMsg);
        delete ctrlMsg;
    }
}

tsch6pSFID_t TschMSF::getSFID() {
    Enter_Method_Silent();
    return pSFID;
}

std::vector<cellLocation_t> TschMSF::pickRandomly(std::vector<cellLocation_t> inputVec, int numRequested)
{
    if ((int) inputVec.size() < numRequested)
        return {};

    // shuffle the array
    std::random_device rd;
    std::mt19937 e{rd()};

    std::shuffle(inputVec.begin(), inputVec.end(), e);

    // copy the picked ones
    std::vector<cellLocation_t> picked = {};
    for (auto i = 0; i < numRequested; i++)
        picked.push_back(inputVec[i]);

//    std::copy(slotOffsets.begin(), slotOffsets.begin() + numSlots, picked.begin());
    return picked;
}

bool TschMSF::slotOffsetAvailable(offset_t slOf) {
    return !pTschLinkInfo->timeOffsetScheduled(slOf)
            && slOf != autoRxCell.timeOffset
            && !slotOffsetReserved(slOf)
            && !slOfScheduled(slOf);
}

std::vector<offset_t> TschMSF::getAvailableSlotsInRange(offset_t start, offset_t end) {
    std::vector<offset_t> slots = {};

    for (int slOf = (int) start; slOf < (int) end; slOf++)
        if (slotOffsetAvailable(slOf))
            slots.push_back(slOf);

    EV_DETAIL << "\nIn range (" << start << ", " << end << ") found available slot offsets: " << slots << endl;
    return slots;
}

std::vector<offset_t> TschMSF::getAvailableSlotsInRange(int slOffsetEnd) {
    return getAvailableSlotsInRange(0, (offset_t) slOffsetEnd);
}

int TschMSF::createCellList(uint64_t destId, std::vector<cellLocation_t> &cellList, int numCells)
{
    Enter_Method_Silent();

    if (cellList.size()) {
        EV_WARN << "cellList should be an empty vector" << endl;
        return -EINVAL;
    }
    if (!reservedTimeOffsets[destId].empty()) {
        EV_ERROR << "reservedTimeOffsets should be empty when creating new cellList,"
                << " is another transaction still in progress?" << endl;
        return -EINVAL;
    }

    std::vector<offset_t> availableSlots = getAvailableSlotsInRange(pSlotframeLength);

    if (!availableSlots.size()) {
        EV_DETAIL << "No available cells found" << endl;
        return -ENOSPC;
    }

    if ((int) availableSlots.size() < numCells) {
        EV_DETAIL << "More cells requested than total available, returning "
                << availableSlots.size() << " cells" << endl;
        for (auto s : availableSlots) {
            cellList.push_back({s, (offset_t) intrand(pNumChannels)});
            reservedTimeOffsets[destId].push_back(s);
        }

        return -EFBIG;
    }

    // Fill cell list with all available slot offsets and random channel offset
    for (auto sl : availableSlots)
        cellList.push_back({sl, (offset_t) intrand(pNumChannels)});

    EV_DETAIL << "Initialized cell list: " << cellList << endl;

    // Select only required number of cells from cell list
    cellList = pickRandomly(cellList, numCells);
    EV_DETAIL << "After picking required number of cells: " << cellList << endl;
    // Block selected cells' slot offsets until 6P transaction finishes
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
    for (auto i = 0; i < numCells && i < (int) pickedCells.size(); i++)
        cellList.push_back(pickedCells[i]);

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

void TschMSF::refreshDisplay() const {
    if (!rplParentId)
        return;

    if (showTxCells || showTxCellCount) {
        std::vector<cellLocation_t> txCells = {};
        txCells = pTschLinkInfo->getDedicatedCells(rplParentId);

        std::ostringstream outCells;

        // Paint dedicated TX cell(s) in red if overlapping cells are detected
        if (hasOverlapping)
            hostNode->getDisplayString().setTagArg("t", 2, "#fc2803");

        if (showTxCellCount)
            outCells << txCells.size();
        else {
            std::sort(txCells.begin(), txCells.end(),
                [](const cellLocation_t c1, const cellLocation_t c2) { return c1.timeOffset < c2.timeOffset; });
            outCells << txCells;
        }

        hostNode->getDisplayString().setTagArg(showQueueUtilization ? "tt" : "t", 0, outCells.str().c_str());
    }

    if (showQueueUtilization) {
        std::ostringstream outUtil;
        outUtil << mac->getQueueUtilization(MacAddress(rplParentId)) << "%";
        hostNode->getDisplayString().setTagArg("t", 0, outUtil.str().c_str()); // show queue utilization above nodes
    }

}

void TschMSF::handleSuccessResponse(uint64_t sender, tsch6pCmd_t cmd, int numCells, std::vector<cellLocation_t> cellList)
{
    if (pTschLinkInfo->getLastKnownType(sender) != MSG_RESPONSE)
        return;

    switch (cmd) {
        case CMD_ADD: {
            // No cells were added
            if (!cellList.size()) {
                EV_DETAIL << "Seems 6P ADD to " << MacAddress(sender) << " failed" << endl;
                return;
//                scheduleRetryAttempt(sender, numCells, MAC_LINKOPTIONS_TX); // TODO: cell options can be other than TX
            }

            removeAutoTxCell(sender);

            EV_DETAIL << "Seems ADD succeeded with cells: " << cellList << ",\nerasing entry from pending transactions" << endl;
//            pendingTransactions.erase(sender);
            emit(s_InitialScheduleComplete, (unsigned long) sender);
            break;
        }
        case CMD_DELETE: {
            // remove cell statistics for cells that have been deleted
            clearCellStats(cellList);
            break;
        }
        case CMD_RELOCATE: {
            if (!cellList.size()) {
                EV_WARN << "Seems RELOCATE failed, worth retrying?" << endl;
            }
            else
                EV_DETAIL << cellList.size() << " successfully relocated" << endl;
            break;
        }
        case CMD_CLEAR: {
            clearCellStats(cellList);
            clearScheduleWithNode(sender);
            break;
        }
        default: EV_DETAIL << "Unsupported 6P command" << endl;
    }

}

void TschMSF::clearScheduleWithNode(uint64_t sender)
{
    auto ctrlMsg = new tsch6topCtrlMsg();
    ctrlMsg->setDestId(sender);
    std::vector<cellLocation_t> deletable = {};
    EV_DETAIL << "Clearing schedule with " << MacAddress(sender) << endl;

    // Clearing cells scheduled with sender (except for auto-cells preserving minimal connectivity)
    // from local schedule
    for (auto &cell : pTschLinkInfo->getCells(sender))
        if (!getCellOptions_isAUTO(std::get<1>(cell)))
            deletable.push_back(std::get<0>(cell));

    if (deletable.size()) {
        EV_DETAIL << "Found cells to delete: " << deletable << endl;
        ctrlMsg->setDeleteCells(deletable);
        pTsch6p->updateSchedule(*ctrlMsg);
    } else
        delete ctrlMsg;

    // Keep AUTO TX cell to pref. parent for better connectivity
//    if (sender == rplParentId)
//        scheduleAutoCell(sender);

//    delete ctrlMsg;
}


// Hacky function to free cells reserved to @param sender when LL ACK is received by 6P layer
void TschMSF::handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells, std::vector<cellLocation_t> *cellList)
{
    reservedTimeOffsets[sender].clear();
    return;
}

void TschMSF::handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells, std::vector<cellLocation_t> cellList)
{
    Enter_Method_Silent();

    auto lastKnownCmd = pTschLinkInfo->getLastKnownCommand(sender);
    EV_DETAIL << "From " << MacAddress(sender) << " received " << code << " for "
            << lastKnownCmd << " with cell list: " << cellList << endl;

    // free the cells that were reserved during the transaction
    reservedTimeOffsets[sender].clear();

    switch (code) {
        case RC_SUCCESS: {
            handleSuccessResponse(sender, lastKnownCmd, numCells, cellList);
            break;
        }
        // Handle all other return codes (RC_RESET, RC_ERROR, RC_VERSION, RC_SFID, ...) as a generic error
        default: {
            clearCellStats(cellList);
            clearScheduleWithNode(sender);
            pTschLinkInfo->resetLink(sender, MSG_RESPONSE);
        }
    }
}

void TschMSF::handleInconsistency(uint64_t destId, uint8_t seqNum) {
    Enter_Method_Silent();
    EV_WARN << "Inconsistency detected" << endl;
    /* Free already reserved cells to avoid race conditions */
    reservedTimeOffsets[destId].clear();

    pTsch6p->sendClearRequest(destId, pTimeout);
}

int TschMSF::getTimeout() {
    Enter_Method_Silent();

    return pTimeout;
}

void TschMSF::addCells(uint64_t nodeId, int numCells, uint8_t cellOptions) {
    if (numCells < 1) {
        EV_WARN << "Invalid number of cells requested - " << numCells << endl;
        return;
    }

    EV_DETAIL << "Trying to add " << numCells << " cell(s) to " << MacAddress(nodeId) << endl;

    if (pTschLinkInfo->inTransaction(nodeId)) {
        EV_WARN << "Can't add cells, currently in another transaction with this node" << endl;
        return;
    }

    std::vector<cellLocation_t> cellList = {};
    createCellList(nodeId, cellList, numCells + pCellListRedundancy);

    if (!cellList.size())
        EV_DETAIL << "No cells could be added to cell list, aborting ADD" << endl;
    else
        pTsch6p->sendAddRequest(nodeId, cellOptions, numCells, cellList, pTimeout);
}

void TschMSF::deleteCells(uint64_t nodeId, int numCells) {
    // If on-demand scheduling of auto cells enabled (IETF MSF draft, 3, p.5)
    // we should remove auto cells if they're not needed
    // legacy option is to prohibit auto cell removal at all, to be factored out after verifying CLSF functionality
    std::vector<cellLocation_t> dedicated = pTschLinkInfo->getDedicatedCells(nodeId);

    if (!dedicated.size()) {
        EV_DETAIL << "No dedicated cells found" << endl;
        // If auto-cell-on-demand is enabled, we can safely delete
        // unused auto cells, as they will be dynamically later
        if (pAutoCellOnDemand) {
            EV_DETAIL << "Auto-cell-on-demand enabled, deleting auto cell" << endl;
            removeAutoTxCell(nodeId);
        }
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

void TschMSF::receiveSignal(cComponent *src, simsignal_t id, cObject *value, cObject *details)
{
    Enter_Method_Silent();

    std::string signalName = getSignalName(id);
//    EV_DETAIL << "got signal " << signalName;

    static std::vector<std::string> namesNeigh = {}; //{"nbSlot-neigh-", "nbTxFrames-neigh-", "nbRxFrames-neigh-"};
    static std::vector<std::string> namesLink = {"nbSlot-link-", "nbTxFrames-link-", "nbRecvdAcks-link-"};

    // this is a hint we should check for new dynamic signals
    if (dynamic_cast<cPostParameterChangeNotification *>(value)) {
        // consider all neighbors we already know
        for (auto const& entry : initialScheduleComplete) {
            // subscribe to all neighbor related signals
            auto macStr = MacAddress(entry.first).str();
            for (auto & name: namesNeigh) {
                auto fullname = name + macStr;
                if (src->isSubscribed(fullname.c_str(), this) == false)
                    src->subscribe(fullname.c_str(), this);
            }

            // subscribe to all link related signals
            for (auto & cell :pTschLinkInfo->getCells(entry.first)) {
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
}

void TschMSF::handleParentChangedSignal(uint64_t newParentId) {
    EV_DETAIL << "RPL parent changed to " << MacAddress(newParentId) << endl;
    emit(rplParentChangedSignal, (unsigned long) newParentId);

    // If RPL parent hasn't been set, we've just joined the DODAG
    if (!rplParentId) {
        rplParentId = newParentId;
        addCells(rplParentId, 1);
        return;
    }

    auto txCells = pTschLinkInfo->getDedicatedCells(rplParentId);
    EV_DETAIL << "Dedicated TX cells currently scheduled with PP: " << txCells << endl;

    /** Clear all negotiated cells and state information with now former PP */
    clearScheduleWithNode(rplParentId);
    pTschLinkInfo->abortTransaction(rplParentId);
    reservedTimeOffsets[rplParentId].clear();
    pTsch6p->sendClearRequest(rplParentId, pTimeout);

    rplParentId = newParentId;

    /** and schedule the same amount of cells (or at least 1) with the new parent */
    addCells(newParentId, (int) txCells.size() > 0 ? txCells.size() : 1);
}

void TschMSF::handlePacketEnqueued(uint64_t dest) {
    auto txCells = pTschLinkInfo->getCellsByType(dest, MAC_LINKOPTIONS_TX);
    EV_DETAIL << "Received MAC notification for a packet enqueued to "
            << MacAddress(dest) << ", cells scheduled to this neighbor:" << txCells << endl;
    if (!txCells.size() && par("autoCellOnDemand").boolValue()) {
        EV_DETAIL << "No TX cell found, scheduling auto cell" << endl;
        scheduleAutoCell(dest);
    }
}


void TschMSF::handleRplRankUpdate(long rank, int numHosts) {
    EV_DETAIL << "Trying to schedule TX cells according to our rank - "
            << rank << " and total number of hosts - " << numHosts << endl;

    if (!rplParentId) {
        EV_WARN << "RPL parent MAC unknown, aborting" << endl;
        return;
    }

    auto currentTxCells = pTschLinkInfo->getDedicatedCells(rplParentId);
    EV_DETAIL << "Found " << currentTxCells.size() << " cells already scheduled with PP: "
            << currentTxCells << endl;

    auto numCellsRequired = numHosts + 3 - ((int) rank) - ((int) currentTxCells.size());
    EV_DETAIL << "Num cells required to schedule - " << numCellsRequired << endl;
    /**
     * "numHosts - rank + 3" corresponds to the number of cells required to handle traffic
     * from the descendant nodes (numHosts - rank) as well as the node itself (+1).
     * Additional +1 to account for the fact, that node closest to the sink are of rank 2 rather than 1.
     * And another +1 to have service rate > arrival rate for stability condition.
     *
     * Each node is assumed to be M/M/1 system with incoming rate of 1 packet per slotframe
     */
    addCells(rplParentId, numCellsRequired);
}

void TschMSF::receiveSignal(cComponent *src, simsignal_t id, long value, cObject *details)
{
    Enter_Method_Silent();

    if (!hasStarted)
        return;

    std::string signalName = getSignalName(id);
    EV_DETAIL << "Got signal - " << signalName << endl;

    if (std::strcmp(signalName.c_str(), "parentChanged") == 0) {
        handleParentChangedSignal(value);
        return;
    }

    if (std::strcmp(signalName.c_str(), "rankUpdated") == 0) {
        auto selfMsg = new cMessage("", DELAY_TEST);
        long updatedRank = value;
        rplRank = value;
        EV_DETAIL << "Set RPL rank inside SF to " << rplRank << endl;
        selfMsg->setContextPointer((long*) &updatedRank);
        // FIXME: Magic numbers
        scheduleAt(simTime() + SimTime(20, SIMTIME_S), selfMsg);
//        handleRplRankUpdate(value, numHosts);
        return;
    }

    if (std::strcmp(signalName.c_str(), "pktEnqueued") == 0) {
        handlePacketEnqueued(value);
        return;
    }

    if (std::strcmp(signalName.c_str(), "oneHopChildJoined") == 0) {
        oneHopRplChildren.push_back(value);
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
        EV_WARN << "could not find neighbor for cell " << linkStr << endl;
        return;
    }

    if (options != 0xFF && getCellOptions_isTX(options))
        updateNeighborStats(neighbor, statisticStr);
}

void TschMSF::updateCellTxStats(cellLocation_t cell, std::string statType) {
    EV_DETAIL << "Updating cell Tx stats" << endl;
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

void TschMSF::updateNeighborStats(uint64_t neighbor, std::string statType) {
    if (nbrStatistic.find(neighbor) == nbrStatistic.end())
        nbrStatistic.insert({neighbor, {0, 0}});

    if (statType == "nbSlot") {
        nbrStatistic[neighbor].NumCellsElapsed++;
        EV_DETAIL << "NumCellsElapsed for " << MacAddress(neighbor) << " now at " << +(nbrStatistic[neighbor].NumCellsElapsed) << endl;
    } else if (statType == "nbTxFrames") {
        nbrStatistic[neighbor].NumCellsUsed++;
        EV_DETAIL << "NumCellsUsed for " << MacAddress(neighbor) << " now at " << +(nbrStatistic[neighbor].NumCellsUsed) << endl;
    }

    if (nbrStatistic[neighbor].NumCellsElapsed >= pMaxNumCells) {
        cMessage* selfMsg = new cMessage();
        selfMsg->setKind(REACHED_MAXNUMCELLS);
        selfMsg->setContextPointer(new MacAddress(neighbor));
        scheduleAt(simTime(), selfMsg);
    }
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
