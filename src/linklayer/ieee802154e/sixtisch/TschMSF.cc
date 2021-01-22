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
//#include "TschMacWaic.h"
#include "../Ieee802154eMac.h"
//#include "ChannelState.h"
//#include "Rpl.h"
#include "Tsch6tischComponents.h"
#include <omnetpp.h>
#include <random>

using namespace tsch;

Define_Module(TschMSF);

TschMSF::TschMSF() :
    routingParentTransactionDelay(3),
    unicastParentCellScheduled(false),
    rplParentId(0),
    hasStarted(false)
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
        pNumMinimalCells = par("numMinCells").intValue();
        disable = par("disable").boolValue();
        internalEvent = new cMessage("SF internal event", UNDEFINED);
        clearReservedCellsTimeout = par("clearReservedCellsTimeout").doubleValue();
        s_InitialScheduleComplete = registerSignal("initial_schedule_complete");
    } else if (stage == 5) {
        interfaceModule = dynamic_cast<InterfaceTable *>(getParentModule()->getParentModule()->getParentModule()->getParentModule()->getSubmodule("interfaceTable", 0));
        pNodeId = interfaceModule->getInterface(1)->getMacAddress().getInt();
        pSlotframeLen = getModuleByPath("^.^.schedule")->par("macSlotframeSize").intValue();
        pTsch6p = (Tsch6topSublayer*) getParentModule()->getSubmodule("sixtop");
        mac = check_and_cast<Ieee802154eMac*>(getModuleByPath("^.^.mac")); // TODO this is very ugly
        mac->subscribe(POST_MODEL_CHANGE, this);

        if (disable)
            return;

        hostNode = getModuleByPath("^.^.^.^.");

        /** Schedule minimal cells for broadcast control traffic [RFC8180, 4.1] */
        scheduleMinimalCells();
        /** And auto RX cell for communication with neighbors [IETF MSF draft, 3]*/
        scheduleAutoRxCell(interfaceModule->getInterface(1)->getMacAddress().formInterfaceIdentifier());

        auto rpl = hostNode->getSubmodule("rpl");
        if (!rpl) {
            EV_WARN << "RPL module not found" << endl;
            return;
        }

        rpl->subscribe("joinedDodag", this);
        rpl->subscribe("parentChanged", this);
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

    cellList.push_back({euiAddr.low() % pSlotframeLen, euiAddr.low() % pNumChannels}); // heuristics
    ctrlMsg->setNewCells(cellList);
    EV_DETAIL << "Scheduling auto RX cell at " << cellList << endl;
    pTsch6p->updateSchedule(*ctrlMsg);
    initialScheduleComplete[pNodeId] = true;
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

void TschMSF::printCellUsage(std::string neighborMac, double usage) {
    std::string usageInfo = std::string(" Usage to/from neighbor ") + neighborMac
            + std::string(usage >= pLimNumCellsUsedHigh ? " exceeds upper" : "")
            + std::string(usage <= pLimNumCellsUsedLow ? " below lower" : "")
            + std::string((usage < pLimNumCellsUsedHigh && usage > pLimNumCellsUsedLow) ? " within" : "")
            + std::string(" limit with ") + std::to_string(usage);

    EV_DETAIL << usageInfo << endl;
//    std::cout << pNodeId << usageInfo << endl;
}

void TschMSF::handleMaxCellsReached(cMessage* msg) {
    EV_DETAIL << "MAX_NUM_CELLS reached, assessing cell usage: " << endl;
    auto neighborMac = (MacAddress*) msg->getContextPointer();
    auto neighbor = neighborMac->getInt();

    // we might get a notification from our own mac
    if (neighbor == pNodeId)
        return;

    double usage = (double) nbrStatistic[neighbor].NumCellsUsed
            / nbrStatistic[neighbor].NumCellsElapsed;

    if (usage >= pLimNumCellsUsedHigh)
        addCells(neighbor, par("cellBandwidthIncrement").intValue());
    if (usage <= pLimNumCellsUsedLow && par("allowCellRemoval").boolValue())
        deleteCells(neighbor, 1);

    printCellUsage(neighborMac->str(), usage);

    // reset values
    nbrStatistic[neighbor].NumCellsUsed = 0; //intrand(pMaxNumCells >> 1);
    nbrStatistic[neighbor].NumCellsElapsed = 0;

    delete neighborMac;
}

void TschMSF::handleDoStart(cMessage* msg) {
    hasStarted = true;
    EV_DETAIL << "MSF has started" << endl;

    if (!par("disableHousekeeping").boolValue())
        scheduleAt(simTime() + SimTime(par("HOUSEKEEPINGCOLLISION_PERIOD").intValue()), new tsch6topCtrlMsg("", HOUSEKEEPING));

    /** Get all nodes that are within communication range of @p nodeId.
    *   Note that this only works if all nodes have been initialized (i.e.
    *   maybe not in init() step 1!) **/
    neighbors = pTsch6p->getNeighborsInRange(pNodeId, mac);

    /* Schedule auto TX cell to all neighbors */
    for (auto & neighbor : neighbors) {
        scheduleAutoCell(neighbor);
        initialScheduleComplete[neighbor] = true;
    }
}

void TschMSF::handleHouskeeping(cMessage* msg) {
    EV_DETAIL << "Performing housekeeping: " << endl;
    scheduleAt(simTime()+ SimTime(par("HOUSEKEEPINGCOLLISION_PERIOD").intValue()), msg);

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

            // TODO: implement this
//            if ((maxPdr->second - cellPdr.second) > pRelocatePdrThres)
//                relocateCell(cellPdr.first, cellPdr.second, maxPdr->second);
        }
    }
}

//void TschMSF::relocateCell(cellLocation_t cell, double cellPdr, double maxPdr) {
//    auto infoStr = std::string("cell (") + std::to_string(cell.timeOffset)
//        + std::string(",") + std::to_string(cell.channelOffset) + std::string(") should be relocated: ")
//        + std::string("it's PDR = ") + std::to_string(cellPdr) + std::string(", max PDR = ")
//        + std::to_string(maxPdr);
////    std::cout << infoStr << endl;
////    EV_DETAIL << infoStr << endl;
//}

void TschMSF::relocateTxCells(cellVector cells) {
    std::vector<cellLocation_t> relocCells;
    std::vector<cellLocation_t> candCells;
    for (auto &c : cells) {
        auto cell = std::get<0>(c);
        auto options = std::get<1>(c);
//        if (pTsch6p->getCellOptions_isMultiple(options));
//            EV_DETAIL << "cell " << cell << " options are multiple, relocate too?" << endl;
        if (options == MAC_LINKOPTIONS_TX)
            relocCells.push_back(cell);
    }

    if (!relocCells.size()) {
        EV_DETAIL << "No TX cells found among " << cells << endl;
        return;
    }
    EV_DETAIL << "Sorted out TX cells to be relocated: " << relocCells << endl;

    auto availableSlots = getAvailableSlotsInRange();

    if (availableSlots.size() < relocCells.size() + par("cellListRedundancy").intValue()) {
        EV_WARN << "Not enough free cells to relocate all currently scheduled cells, aborting" << endl;
        return;
    }

    auto slOffsets = pickSlotOffsets(availableSlots, relocCells.size() + par("cellListRedundancy").intValue());
    for (auto s: slOffsets)
        candCells.push_back({s, (offset_t) intrand(pNumChannels)});

    EV_DETAIL << "New candidate cells selected for relocated ones: " << candCells << endl;

    pTsch6p->sendRelocationRequest(rplParentId, MAC_LINKOPTIONS_TX, relocCells.size(), relocCells, candCells, pTimeout);
}

void TschMSF::handleMessage(cMessage* msg) {
    Enter_Method_Silent();

    if (!msg->isSelfMessage())
        return;

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
            handleHouskeeping(msg);
            return;
        }
        case SCHEDULE_AUTO_TX: {
            if (!rplParentId) {
                EV_WARN << "Parent MAC undefined" << endl;
                break;
            }
            if (!unicastParentCellScheduled) {
                EV_DETAIL << "Retrying scheduling dedicated TX cells with preferred parent" << endl;
                int numCellsRequired = ((MsfControlInfo *) msg->getControlInfo())->getNumDedicatedCells();
                addCells(rplParentId, numCellsRequired);
            }


//            EV_DETAIL << "Scheduling auto TX cell with RPL parent" << endl;
//
//            if (!unicastParentCellScheduled) {
//                // reset hanging link
//                pTschLinkInfo->resetLink(rplParentId, MSG_RESPONSE);
//                reservedTimeOffsets[rplParentId].clear();
//
//                auto timeout = simTime() + uniform(1, 2) * routingParentTransactionDelay;
//
//                EV_DETAIL << "Auto TX to parent " << MacAddress(rplParentId) << " not yet scheduled, attempting 6P ADD, "
//                        << "next retry at " << timeout << endl;
//                addCells(rplParentId, 1);
//                scheduleAt(timeout, new cMessage("", SCHEDULE_AUTO_TX));
//            } else
//                EV_DETAIL << "Already scheduled" << endl;

            break;
        }
        case RESERVED_CELLS_TIMEOUT: {
            auto destId = ((MsfControlInfo*) msg->getControlInfo())->getNodeId();

            EV_DETAIL << "Reserved cells: " << endl;
            for (auto res : reservedTimeOffsets) {
                EV_DETAIL << res.first << ": " << res.second << endl;
            }

            EV_DETAIL << "Timeout popped for reserved cells to "
                    << destId << ": " << reservedTimeOffsets[destId] << endl;
            reservedTimeOffsets[destId].clear();
            EV_DETAIL << " cleared associated cells: " << reservedTimeOffsets[destId] << endl;

            break;
        }
        default: EV_ERROR << "Unknown message received: " << msg << endl;
    }
    delete msg;
}

void TschMSF::scheduleAutoCell(uint64_t neighbor) {
    auto ctrlMsg = new tsch6topCtrlMsg();
    ctrlMsg->setDestId(neighbor);
    ctrlMsg->setCellOptions(MAC_LINKOPTIONS_TX | MAC_LINKOPTIONS_SHARED | MAC_LINKOPTIONS_SRCAUTO);

    auto neighborIntfIdent = MacAddress(neighbor).formInterfaceIdentifier();

    uint32_t nbruid = neighborIntfIdent.low();

//    EV_DETAIL << "Neighbor " << MacAddress(neighbor) << " low - " << std::to_string(nbruid)
//        << ", possible auto cell at " << std::to_string(nbruid % pSlotframeLen) << ", " << std::to_string(nbruid % pNumChannels) << endl;

    std::vector<cellLocation_t> cellList;
//    cellList.push_back({1 + saxHash(pSlotframeLen - 1, neighborIntfIdent), saxHash(16, neighborIntfIdent)});

    cellList.push_back({nbruid % pSlotframeLen, nbruid % pNumChannels});
    ctrlMsg->setNewCells(cellList);

    EV_DETAIL << "Scheduling auto TX cell at " << cellList << " to neighbor - " << MacAddress(neighbor) << endl;

    pTschLinkInfo->addLink(neighbor, false, 0, 0);
    pTschLinkInfo->addCell(neighbor, cellList[0], MAC_LINKOPTIONS_TX | MAC_LINKOPTIONS_SHARED | MAC_LINKOPTIONS_SRCAUTO);
    pTsch6p->updateSchedule(*ctrlMsg);
    delete ctrlMsg;
}

void TschMSF::scheduleMinimalCells() {
    EV_DETAIL << "Scheduling minimal cells for broadcast messages: " << endl;
    auto ctrlMsg = new tsch6topCtrlMsg();
    auto macBroadcast = 0xFFFFFFFFFFFF;
    ctrlMsg->setDestId(macBroadcast);
    auto cellOpts = MAC_LINKOPTIONS_TX | MAC_LINKOPTIONS_RX | MAC_LINKOPTIONS_SHARED;
    ctrlMsg->setCellOptions(cellOpts);

    std::vector<cellLocation_t> cellList = {};
    for (offset_t i = 0; i < pNumMinimalCells + 1; i++)
        cellList.push_back({i, 0});

    ctrlMsg->setNewCells(cellList);
    pTschLinkInfo->addLink(macBroadcast, false, 0, 0);
    for (auto cell : cellList)
        pTschLinkInfo->addCell(macBroadcast, cell, cellOpts);

    pTsch6p->updateSchedule(*ctrlMsg);
    delete ctrlMsg;
}


void TschMSF::removeAutoTxCell(uint64_t neighbor) {
    std::vector<cellLocation_t> cellList;

    for (auto &cell: pTschLinkInfo->getCells(neighbor))
        if (std::get<1>(cell) != 0xFF && getCellOptions_isAUTO(std::get<1>(cell)))
            cellList.push_back(std::get<0>(cell));

    if (cellList.size()) {
        EV_DETAIL << "Removing auto TX cell to neighbor - " << MacAddress(neighbor) << endl;
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

bool TschMSF::checkValidSlotRangeBounds(uint16_t start, uint16_t end) {
    return !(start > pSlotframeLen || end > pSlotframeLen || end <= start);
}

std::vector<offset_t> TschMSF::pickSlotOffsets(std::vector<offset_t> availableSlots, int numRequested) {
    // shuffle the array
    std::random_device rd;
    std::mt19937 e{rd()};

    std::shuffle(availableSlots.begin(), availableSlots.end(), e);

    // copy the picked ones
    std::vector<offset_t> picked = {};
    for (auto i = 0; i < numRequested; i++)
        picked.push_back(availableSlots[i]);

//    std::copy(slotOffsets.begin(), slotOffsets.begin() + numSlots, picked.begin());
    return picked;
}

bool TschMSF::slotOffsetAvailable(offset_t slOf) {
    return !pTschLinkInfo->timeOffsetScheduled(slOf) && slOf != autoRxSlOffset && !timeOffsetReserved(slOf);
}

std::vector<offset_t> TschMSF::getAvailableSlotsInRange(offset_t start, offset_t end) {
    std::vector<offset_t> slots = {};

    for (auto slOf = start; slOf < end; slOf++) {
        if (slotOffsetAvailable(slOf))
            slots.push_back(slOf);
    }

    EV_DETAIL << "In range (" << start << ", " << end << ") found available slot offsets: " << slots << endl;
    return slots;
}


std::vector<offset_t> TschMSF::getAvailableSlotsInRange(int slOffsetEnd) {
    return getAvailableSlotsInRange(pNumMinimalCells + 1, slOffsetEnd);
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
                << " is another transaction still in progress? " << endl;
//                << " scheduling timeout event to clear them at "
//                << simTime() + clearReservedCellsTimeout << endl;

//        EV_DETAIL << "Currently reserved slot offsets: " << reservedTimeOffsets[destId] << endl;
//        auto clearReserved = new cMessage("", RESERVED_CELLS_TIMEOUT);
//        clearReserved->setControlInfo(new MsfControlInfo(destId));
//
//        scheduleAt(simTime() + clearReservedCellsTimeout, clearReserved);
        return -EINVAL;
    }

    // check available slot offsets
    std::vector<offset_t> availableSlots = getAvailableSlotsInRange();

    if (availableSlots.size() < numCells) {
        EV_DETAIL << "More cells requested than total available, returning "
                << availableSlots.size() << " cells" << endl;
        for (auto s : availableSlots) {
            cellList.push_back({s, (offset_t) intrand(pNumChannels)});
            reservedTimeOffsets[destId].push_back(s);
        }

        return -EFBIG;
    }

    // prepare cell list containing free cells
    std::vector<offset_t> picked = pickSlotOffsets(availableSlots, numCells);
    for (auto p : picked) {
        cellList.push_back({p, (offset_t) intrand(pNumChannels)});
        reservedTimeOffsets[destId].push_back(p);
    }

    if (!cellList.size())
        return -ENOSPC;

    EV_DETAIL << "Created cell list - " << cellList << endl;
    return 0;
}

int TschMSF::pickCells(uint64_t destId, std::vector<cellLocation_t> &cellList,
                        int numCells, bool isRX, bool isTX, bool isSHARED) {
    Enter_Method_Silent();

    if (cellList.size() == 0)
        return -EINVAL;

    EV_DETAIL << "Picking cells from list: " << cellList << endl;

    std::vector<cellLocation_t> pickedCells = {};

    // TODO: refactor this
    for (auto cell : cellList) {
        unsigned int slOf = cell.timeOffset;
        EV_DETAIL << "proposed cell " << cell;
        if (slotOffsetAvailable(slOf))
        {
            EV_DETAIL << " available" << endl;
            /* cell is still available, pick it. */
            pickedCells.push_back(cell);

        } else
            EV_DETAIL << " unavailable" << endl;
    }
    cellList.clear();
    for (auto i = 0; i < numCells && i < pickedCells.size(); i++)
        cellList.push_back(pickedCells[i]);

    EV_DETAIL << "Cell list after picking: " << cellList << endl;

    if (pickedCells.size() == 0)
        return -ENOSPC;

    if (pickedCells.size() < numCells)
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

void TschMSF::handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells, std::vector<cellLocation_t> *cellList)
{
    EV_WARN << "Deprecated handleResponse() invoked" << endl;
}

void TschMSF::checkDedicatedCellScheduled(uint64_t sender, std::vector<cellLocation_t> cellList) {
    if (sender != rplParentId || unicastParentCellScheduled)
        return;

    if (cellList.empty()) {
        EV_DETAIL << "Last 6P ADD to preferred parent - " << MacAddress(sender) << " failed, retrying" << endl;
        addCells(sender, 1);
        return;
    }

    unicastParentCellScheduled = true;
    EV_DETAIL << "Scheduled " << cellList << " as dedicated TX cell(s) to our preferred parent - "
            << MacAddress(sender) << endl;
    removeAutoTxCell(sender);
}

void TschMSF::handleSuccessResponse(uint64_t sender, tsch6pCmd_t cmd, int numCells, std::vector<cellLocation_t> cellList)
{
    if (pTschLinkInfo->getLastKnownType(sender) != MSG_RESPONSE)
        return;

    switch (cmd) {
        case CMD_ADD: {
            checkDedicatedCellScheduled(sender, cellList);
            emit(s_InitialScheduleComplete, (unsigned long) sender);
            break;
        }
        case CMD_DELETE: {
            // remove cell statistics for cells that have been deleted
            clearCellStats(cellList);
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
    std::vector<cellLocation_t> deletableCells = {};
    auto neighbMac = MacAddress(sender);
//    auto neighbLinks = schedule->getDedicatedLinksForNeighbor(neighbMac);
//
//    EV_DETAIL << "Clearing state with " << neighbMac << ", found dedicated links: " << neighbLinks << endl;
//
//    for (auto link : neighbLinks)
//        deletableCells.push_back({(offset_t) link->getSlotOffset(), (offset_t) link->getChannelOffset()});

    for (auto &cell : pTschLinkInfo->getCells(sender))
        if (!getCellOptions_isAUTO(std::get<1>(cell)))
            deletableCells.push_back(std::get<0>(cell));

    if (deletableCells.size() > 0) {
        EV_DETAIL << "Clearing schedule with " << neighbMac << ", found cells to delete: " << deletableCells << endl;
        ctrlMsg->setDeleteCells(deletableCells);
        pTsch6p->updateSchedule(*ctrlMsg);
    }

    pTschLinkInfo->resetLink(sender, MSG_RESPONSE);
    delete ctrlMsg;
}


void TschMSF::handleFailedTransaction(uint64_t sender, tsch6pCmd_t cmd)
{
    EV_DETAIL << "Handling failed " << cmd << " sent to " << MacAddress(sender) << endl;
    switch (cmd) {
        case CMD_ADD: {
            checkDedicatedCellScheduled(sender, {});
            break;
        }
        case CMD_CLEAR: {
            if (sender == rplFormerParentId)
                EV_DETAIL << "Clearing former preferred parent's schedule failed?" << endl;
            break;
        }
    }

}

void TschMSF::handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells, std::vector<cellLocation_t> cellList)
{
    Enter_Method_Silent();
    auto senderMac = MacAddress(sender);

    auto lastKnownCmd = pTschLinkInfo->getLastKnownCommand(sender);

    EV_DETAIL << "Handling " << code << " for " << lastKnownCmd << " and cell list: " << cellList << endl;

    // free the cells that were reserved during the transaction
    reservedTimeOffsets[sender].clear();

    switch (code) {
        case RC_SUCCESS: {
            handleSuccessResponse(sender, lastKnownCmd, numCells, cellList);
            break;
        }
        // Skip these for now
        case RC_BUSY:
        case RC_LOCKED: {
            break;
        }
        case RC_RESET: {
            clearCellStats(cellList);
            clearScheduleWithNode(sender);
            handleFailedTransaction(sender, lastKnownCmd);
            // no break here is intentional!
        }
        // Handle all other return codes (RC_ERROR, RC_VERSION, RC_SFID, ...) as error
        default: {
            clearCellStats(cellList);
            clearScheduleWithNode(sender);
        }
    }
}



void TschMSF::handleInconsistency(uint64_t destId, uint8_t seqNum) {
    Enter_Method_Silent();
    EV_WARN << "Inconsistency detected" << endl;
    /*free reserved cells already to avoid race conditions*/
    reservedTimeOffsets[destId].clear();

    pTsch6p->sendClearRequest(destId, pTimeout);
}

int TschMSF::getTimeout() {
    Enter_Method_Silent();

    return pTimeout;
}

uint64_t TschMSF::checkInTransaction() {
    for (auto nbr: neighbors)
       if (pTschLinkInfo->inTransaction(nbr))
           return nbr;

    return 0;
}

void TschMSF::retrySchedulingDedicatedCells(int numCells) {
    auto timeout = simTime() + uniform(1, 1.5) * pTimeout;
    EV_DETAIL << "Dedicated cell still not scheduled, next attempt at " << timeout << " s" << endl;
    auto ci = new MsfControlInfo();
    ci->setNumDedicatedCells(numCells);
    auto timeoutMsg = new cMessage("", SCHEDULE_AUTO_TX);
    timeoutMsg->setControlInfo(ci);
    scheduleAt(timeout, timeoutMsg);
}

void TschMSF::addCells(uint64_t nodeId, int numCells) {
    EV_DETAIL << "Trying to add " << numCells << " cell(s) to neighbor " << MacAddress(nodeId) << endl;

    /* Attempt only if we're not in the process of another transaction */
    if (pTschLinkInfo->inTransaction(nodeId)) {
        EV_WARN << "Can't add cells, currently in another transaction with this node" << endl;
        if (nodeId == rplParentId && !unicastParentCellScheduled)
            retrySchedulingDedicatedCells(numCells);

        return;
    }

    std::vector<cellLocation_t> cellList = {};

    createCellList(nodeId, cellList, numCells);
    if (!cellList.size())
        EV_DETAIL << "No cells could be added to cell list, aborting 6P ADD" << endl;
    else
        pTsch6p->sendAddRequest(nodeId, MAC_LINKOPTIONS_TX, cellList.size(), cellList, pTimeout);
}

void TschMSF::deleteCells(uint64_t nodeId, int numCells) {
    auto schedule = check_and_cast<TschSlotframe*>(getModuleByPath("^.^.schedule"));
    auto neighbLinks = schedule->allTxLinks(MacAddress(nodeId));

    if (!neighbLinks.size()) {
        EV_WARN << "Got request to delete " << numCells << " cell(s) to "
                << MacAddress(nodeId) << "but no dedicated TX cells found" << endl;
        return;
    }

    // do not delete last dedicated link/cell, as it may seriously impede connectivity
    if (neighbLinks.size() < 2)
        return;

    std::vector<cellLocation_t> cellList = {};

    if (neighbLinks.size() - numCells < 1) {
        EV_WARN << "Requested to delete more cells than available" << endl;
        return;
    }

    for (auto i = 0; i < numCells; i++)
        cellList.push_back({(offset_t) neighbLinks[i]->getSlotOffset(), (offset_t) neighbLinks[i]->getChannelOffset()});

    EV_DETAIL << "Cells chosen for deletion: " << cellList << endl;
    pTsch6p->sendDeleteRequest(nodeId, MAC_LINKOPTIONS_TX, numCells, cellList, pTimeout);
}

bool TschMSF::timeOffsetReserved(offset_t timeOffset) {
    std::map<uint64_t, std::vector<offset_t>>::iterator nodes;

    for (nodes = reservedTimeOffsets.begin(); nodes != reservedTimeOffsets.end(); ++nodes) {
        if (timeOffsetReservedByNode(nodes->first, timeOffset))
            return true;
    }

    return false;
}

bool TschMSF::timeOffsetReservedByNode(uint64_t nodeId, offset_t timeOffset) {
    std::vector<offset_t> offsetSet = reservedTimeOffsets[nodeId];
    return std::find(offsetSet.begin(), offsetSet.end(), timeOffset) != offsetSet.end();
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


void TschMSF::handleDodagJoinedSignal(uint64_t parentId) {
    EV_DETAIL << "Trying to schedule dedicated TX cell with RPL parent - " << MacAddress(parentId) << endl;
    rplParentId = parentId;
    addCells(parentId, 1);
}

void TschMSF::handleParentChangedSignal(uint64_t newParentId) {
    EV_DETAIL << "RPL parent changed to " << MacAddress(newParentId) << endl;
    rplFormerParentId = rplParentId;
    rplParentId = newParentId;

    /** Get number of dedicated TX cells scheduled with former RPL parent */
    auto numScheduledCells = pTschLinkInfo->getCells(rplFormerParentId).size();

    EV_DETAIL << "Dedicated cells found - " << numScheduledCells << endl;

    unicastParentCellScheduled = false;

    /** and schedule the same amount with the new parent */
    addCells(rplParentId, numScheduledCells);

//    /** Clear these cells from former parent's schedule */
//    pTsch6p->sendClearRequest(rplFormerParentId, pTimeout);
}

void TschMSF::receiveSignal(cComponent *src, simsignal_t id, long value, cObject *details)
{
    Enter_Method_Silent();

    std::string signalName = getSignalName(id);
    EV_DETAIL << "Got signal " << signalName << endl;

    if (!hasStarted)
        return;

    if (std::strcmp(signalName.c_str(), "joinedDodag") == 0) {
        handleDodagJoinedSignal(value);
        return;
    }

    if (std::strcmp(signalName.c_str(), "parentChanged") == 0) {
        handleParentChangedSignal(value);
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

    if (!getCellOptions_isAUTO(options))
        updateCellTxStats(cell, statisticStr);

    if (options != 0xFF && getCellOptions_isTX(options))
        updateNeighborStats(neighbor, statisticStr);
}

void TschMSF::updateCellTxStats(cellLocation_t cell, std::string statType) {
    EV_DETAIL << "Updating cell Tx stats" << endl;
    if (cellStatistic.find(cell) == cellStatistic.end())
        cellStatistic.insert({cell, {0, 0}});

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
        EV_DETAIL << "NumCellsElapsed++ for " << MacAddress(neighbor) << " now at " << +(nbrStatistic[neighbor].NumCellsElapsed) << endl;
    } else if (statType == "nbTxFrames") {
        nbrStatistic[neighbor].NumCellsUsed++;
        EV_DETAIL << "NumCellsUsed++ for " << MacAddress(neighbor) << " now at " << +(nbrStatistic[neighbor].NumCellsUsed) << endl;
        if (nbrStatistic[neighbor].NumCellsUsed >= pMaxNumCells) {
            cMessage* selfMsg = new cMessage();
            selfMsg->setKind(REACHED_MAXNUMCELLS);
            selfMsg->setContextPointer(new MacAddress(neighbor));
            scheduleAt(simTime(), selfMsg);
        }
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
