/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 * Stores and handles information about all links maintained by
 * this node.
 *
 * Copyright (C) 2021  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
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

#include "TschLinkInfo.h"
#include "../../../common/TschUtils.h"
#include "inet/linklayer/common/MacAddress.h"

Define_Module(TschLinkInfo);

TschLinkInfo::TschLinkInfo() {
}

TschLinkInfo::~TschLinkInfo() {
    for (auto& entry: linkInfo) {
        cancelAndDelete(entry.second.tom);
    }
    linkInfo.clear();
}

void TschLinkInfo::initialize(int stage) {
    if (stage == 0) {
        sublayerControlOut = findGate("sublayerControlOut");
    }
//    for (auto li : linkInfo)
//        WATCH_VECTOR(std::get<1>(li).scheduledCells);

    numScheduleClears = 0;

    WATCH_MAP(linkInfo);
    WATCH(numScheduleClears);
}

bool TschLinkInfo::linkInfoExists(uint64_t nodeId) {
    Enter_Method_Silent();

    auto it = linkInfo.find(nodeId);

    if (it == linkInfo.end()) {
        return false;
    }
    return true;
}

std::vector<uint64_t> TschLinkInfo::getLinks() {
    std::vector<uint64_t> nodeIds;

    for(auto i = linkInfo.begin(); i != linkInfo.end(); ++i) {
        nodeIds.push_back(i->first);
    }

    return nodeIds;
}


int TschLinkInfo::addLink(uint64_t nodeId, bool inTransaction,
                          simtime_t transactionTimeout, uint8_t lastKnownSeqNum) {
    Enter_Method_Silent();

    if (linkInfoExists(nodeId)) {
        /* don't add more than one entry for nodeId */
        return -EEXIST;
    }

    linkInfo[nodeId] = {};
    linkInfo[nodeId].nodeId = nodeId;
    linkInfo[nodeId].inTransaction= inTransaction;
    linkInfo[nodeId].tom = new tschLinkInfoTimeoutMsg();
    linkInfo[nodeId].tom->setNodeId(nodeId);
    linkInfo[nodeId].lastKnownSeqNum = lastKnownSeqNum;
    linkInfo[nodeId].lastKnownType = MSG_NONE;
    linkInfo[nodeId].lastKnownCommand = CMD_NONE;

    if (inTransaction)
        startTimeoutTimer(nodeId, transactionTimeout);

    return 0;
}

void TschLinkInfo::resetLink(uint64_t nodeId, tsch6pMsg_t lastKnownType) {
    resetSeqNum(nodeId);
    abortTransaction(nodeId);
    clearCells(nodeId);
    setLastKnownType(nodeId, lastKnownType);
    setLastLinkOption(nodeId, MAC_LINKOPTIONS_NONE);
}

void TschLinkInfo::revertLink(uint64_t nodeId, tsch6pMsg_t lastKnownType) {
    abortTransaction(nodeId);
    setLastKnownType(nodeId, lastKnownType);
    setLastLinkOption(nodeId, MAC_LINKOPTIONS_NONE);
    EV_DETAIL << "Reverted link with " << MacAddress(nodeId) << endl;
}

bool TschLinkInfo::inTransaction(uint64_t nodeId) {
    Enter_Method_Silent();

    if (linkInfoExists(nodeId))
        return linkInfo[nodeId].inTransaction;

    return false;
}

int TschLinkInfo::setInTransaction(uint64_t nodeId) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId))
        return -EINVAL;

    linkInfo[nodeId].inTransaction = true;

    return 0;
}

int TschLinkInfo::setInTransaction(uint64_t nodeId, simtime_t transactionTimeout) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId))
        return -EINVAL;

    linkInfo[nodeId].inTransaction = true;
    startTimeoutTimer(nodeId, transactionTimeout);

    return 0;
}

int TschLinkInfo::abortTransaction(uint64_t nodeId) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId)) {
        return -EINVAL;
    }

    if (linkInfo[nodeId].inTransaction) {
        EV_DETAIL << "Closing " << getLastKnownCommand(nodeId) << " with " << inet::MacAddress(nodeId) << endl;
        if (linkInfo[nodeId].tom) {
            linkInfo[nodeId].tom->setSeqNum(linkInfo[nodeId].lastKnownSeqNum);
            cancelEvent(linkInfo[nodeId].tom);
        }
        linkInfo[nodeId].inTransaction = false;
        linkInfo[nodeId].relocationCells.clear();
        linkInfo[nodeId].lastLinkOption = MAC_LINKOPTIONS_NONE;
    }

    return 0;
}

int TschLinkInfo::addCell(uint64_t nodeId, cellLocation_t cell, uint8_t linkOption) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId) || isCellAlreadyScheduled(cell.timeOffset, nodeId))
        return -EINVAL;

    std::tuple<cellLocation_t, uint8_t> cellTuple = std::make_tuple(cell, linkOption);
    linkInfo[nodeId].scheduledCells.push_back(cellTuple);

    return 0;
}

bool TschLinkInfo::isCellAlreadyScheduled(offset_t slotOffset, uint64_t neighborId) {
    for (auto link : linkInfo[neighborId].scheduledCells)
        if (std::get<0>(link).timeOffset == slotOffset)
            return true;

    return false;
}

int TschLinkInfo::addCells(uint64_t nodeId, const std::vector<cellLocation_t> &cellList, uint8_t linkOption)
{
    Enter_Method_Silent();

    EV_DETAIL << "Adding cells: " << cellList << " " << printLinkOptions(linkOption) << endl;

    if (!linkInfoExists(nodeId))
        return -EINVAL;

    auto it = cellList.begin();
    for(; it != cellList.end(); ++it)
        addCell(nodeId, *it, linkOption);

    return 0;
}

cellVector TschLinkInfo::getCells(uint64_t nodeId) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId))
        return {};

    cellVector cv(linkInfo[nodeId].scheduledCells);
    return cv;
}

bool TschLinkInfo::sharedTxScheduled(uint64_t nodeId) {
    for (auto link: linkInfo[nodeId].scheduledCells) {
        auto opts = std::get<1>(link);
        if (getCellOptions_isSHARED(opts) && getCellOptions_isTX(opts))
            return true;
    }
    return false;
}

std::vector<cellLocation_t> TschLinkInfo::getSharedCellsWith(uint64_t nodeId) {
    std::vector<cellLocation_t> sharedCells = {};

    for (auto link: linkInfo[nodeId].scheduledCells) {
        auto opts = std::get<1>(link);
        if (getCellOptions_isSHARED(opts) && getCellOptions_isTX(opts))
            sharedCells.push_back(std::get<0>(link));
    }
    return sharedCells;
}

// TODO: refactor to be more clear
std::vector<cellLocation_t> TschLinkInfo::getCellLocations(uint64_t nodeId) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId))
        return {};

    std::vector<cellLocation_t> res = {};
    // DO NOT take the auto-cell, since it may just leave remaining queued packets there for a long time
    for (auto cell_tuple: linkInfo[nodeId].scheduledCells)
        if (!getCellOptions_isAUTO(std::get<1>(cell_tuple)))
            res.push_back(std::get<0>(cell_tuple));

    return res;
}

cellVector TschLinkInfo::getMinimalCells() {
    return linkInfo[inet::MacAddress::BROADCAST_ADDRESS.getInt()].scheduledCells;
}

cellListVector TschLinkInfo::getMinimalCells(offset_t slotOffset) {
    auto minCellLinks = getMinimalCells();
    cellListVector cv = {};
    for (auto mc : minCellLinks) {
        if (std::get<0>(mc).timeOffset == slotOffset)
            cv.push_back(std::get<0>(mc));
    }

    return cv;
}

std::vector<cellLocation_t> TschLinkInfo::getCellList(uint64_t nodeId) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId))
        return {};

    std::vector<cellLocation_t> cellList = {};
    for (auto cellInfo : linkInfo[nodeId].scheduledCells)
        cellList.push_back(std::get<0>(cellInfo));

    return cellList;
}

//std::vector<cellLocation_t> TschLinkInfo::getDedicatedTxCells(uint64_t nodeId) {
//    std::vector<cellLocation_t> res = {};
//
//    for (auto link : linkInfo[nodeId].scheduledCells) {
//        auto cellOp = std::get<1>(link);
//        if (getCellOptions_isTX(cellOp) && !getCellOptions_isAUTO(cellOp) && !getCellOptions_isSHARED(cellOp))
//            res.push_back(std::get<0>(link));
//    }
//
//    return res;
//}

std::vector<cellLocation_t> TschLinkInfo::getDedicatedCells(uint64_t nodeId, bool requireRx) {
    std::vector<cellLocation_t> res = {};

    for (auto link : linkInfo[nodeId].scheduledCells) {
        auto cellOp = std::get<1>(link);
        if (getCellOptions_isAUTO(cellOp) || getCellOptions_isSHARED(cellOp))
            continue;

        if (requireRx && getCellOptions_isRX(cellOp))
            res.push_back(std::get<0>(link));

        if (!requireRx && getCellOptions_isTX(cellOp))
            res.push_back(std::get<0>(link));
    }

    return res;
}

std::vector<cellLocation_t> TschLinkInfo::getCellsByType(uint64_t nodeId, uint8_t requiredCellType) {
    std::vector<cellLocation_t> res = {};

    for (auto link : linkInfo[nodeId].scheduledCells) {
        auto cellOp = std::get<1>(link);
        if (getCellOptions_isTX(cellOp) && requiredCellType == MAC_LINKOPTIONS_TX)
            res.push_back(std::get<0>(link));
        if (getCellOptions_isRX(cellOp) && requiredCellType == MAC_LINKOPTIONS_RX)
            res.push_back(std::get<0>(link));
    }

    return res;
}



uint8_t TschLinkInfo::getCellOptions(uint64_t nodeId, cellLocation_t candidate) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId)) {
        return 0xFF;
    }

    auto it = std::find_if(linkInfo[nodeId].scheduledCells.begin(), linkInfo[nodeId].scheduledCells.end(),
                           [candidate](const std::tuple<cellLocation_t, uint8_t> & t) -> bool {
                             return std::get<0>(t) == candidate; });

    if (it != linkInfo[nodeId].scheduledCells.end()) {
        return std::get<1>(*it);
    }

    return 0xFF;
}


int TschLinkInfo::getNumCells(uint64_t nodeId) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId)) {
        return 0;
    }

    return linkInfo[nodeId].scheduledCells.size();
}

uint64_t TschLinkInfo::getNodeOfCell(cellLocation_t candidate) {
    /* loop through all links */
    for (auto & info: linkInfo) {

        auto it = std::find_if(info.second.scheduledCells.begin(), info.second.scheduledCells.end(),
               [candidate](const std::tuple<cellLocation_t, uint8_t> & t) -> bool {
                 return std::get<0>(t) == candidate && !getCellOptions_isSHARED(std::get<1>(t));
            }
        );

        if (it != info.second.scheduledCells.end())
            return info.first;
    }

    return 0;
}


void TschLinkInfo::clearCells(uint64_t nodeId) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId)) {
        EV_WARN << "Instructed to clear but no linkInfo exists" << endl;
        return;
    }

    EV_INFO << "Clearing cells scheduled with " << inet::MacAddress(nodeId)
        << "\nBefore erasure: " << linkInfo[nodeId].scheduledCells << endl;

    linkInfo[nodeId].scheduledCells.erase(
        std::remove_if( linkInfo[nodeId].scheduledCells.begin(), linkInfo[nodeId].scheduledCells.end(),
            [] (decltype(linkInfo[nodeId].scheduledCells)::value_type link) -> bool {
                return !getCellOptions_isAUTO(std::get<1>(link));
            }
        ),
        linkInfo[nodeId].scheduledCells.end()
    );

    EV_DETAIL << "After: " << linkInfo[nodeId].scheduledCells << endl;

    numScheduleClears++;
}

void TschLinkInfo::deleteCells(uint64_t nodeId, const std::vector<cellLocation_t> &cellList, uint8_t linkOption) {
    Enter_Method_Silent();

    if (cellList.empty())
        return;

    EV_DETAIL << "Deleting cells scheduled with " << inet::MacAddress(nodeId) << " : " << cellList << endl;

    if (!linkInfoExists(nodeId)) {
        EV_WARN << "No linkInfo found" << endl;
        return;
    }

    cellVector *scheduledCells = &(linkInfo[nodeId].scheduledCells); // TODO: replace with a proper getter method

    auto it = cellList.begin();
    for(; it != cellList.end(); ++it)
    {
        auto elem = std::find_if(scheduledCells->begin(), scheduledCells->end(),
                        [it](std::tuple<cellLocation_t, uint8_t> link) -> bool {
            return std::get<0>(link) == *it;
        });

        if (elem != scheduledCells->end())
            scheduledCells->erase(elem);
        else
            EV_WARN << "Instructed to delete cell " << *it << " but not found" << endl;
    }

    EV_DETAIL << "Scheduled cells after erasure:\n" << *scheduledCells << endl;
}

bool TschLinkInfo::timeOffsetScheduled(offset_t timeOffset) {
    Enter_Method_Silent();

    /* loop through all links */
    for(auto & info: linkInfo) {
        auto it = std::find_if(info.second.scheduledCells.begin(), info.second.scheduledCells.end(),
          [timeOffset] (const std::tuple<cellLocation_t, uint8_t> & t) -> bool {
            return std::get<0>(t).timeOffset == timeOffset;
        });

        if (it != info.second.scheduledCells.end()) {
            return true;
        }
    }

    return false;
}

bool TschLinkInfo::cellsInSchedule(uint64_t nodeId, std::vector<cellLocation_t> &cellList, uint8_t linkOption)
{
    Enter_Method_Silent();

    bool inSchedule = false;

    if (linkInfo.find(nodeId) == linkInfo.end())
        return false;

    inSchedule = true;
    cellVector scheduled = linkInfo[nodeId].scheduledCells;
//    std::vector<cellLocation_t>::iterator it;
    for (auto it = cellList.begin(); it != cellList.end() && inSchedule; ++it) {
        auto currCell = std::make_tuple(*it, linkOption);
        inSchedule = std::find(scheduled.begin(), scheduled.end(), currCell) != scheduled.end();
    }

    return inSchedule;
}

int TschLinkInfo::setRelocationCells(uint64_t nodeId,
                                     std::vector<cellLocation_t> &cellList,
                                     uint8_t linkOption) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId))
        return -EINVAL;

    if (linkInfo[nodeId].relocationCells.size() || linkInfo[nodeId].inTransaction)
        return -EINVAL;

    for (auto cell: cellList) {
        auto cellTuple = std::make_tuple(cell, linkOption);
        linkInfo[nodeId].relocationCells.push_back(cellTuple);
    }

    return 0;
}

std::vector<cellLocation_t> TschLinkInfo::getRelocationCells(uint64_t nodeId) {
    Enter_Method_Silent();

    std::vector<cellLocation_t> result = {};
    if (linkInfoExists(nodeId)) {
        for (std::tuple<cellLocation_t, uint8_t> cell:
                                            linkInfo[nodeId].relocationCells ) {
            result.push_back(std::get<0>(cell));
        }
    }

    return result;
}

int TschLinkInfo::relocateCells(uint64_t nodeId, std::vector<cellLocation_t> &newCells, uint8_t linkOption)
{
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId)) {
        EV_DETAIL << "Cannot relocate, link info for this node doesn't exist!" << endl;
        return -EINVAL;
    }

    if (!linkInfo[nodeId].inTransaction || (linkInfo[nodeId].inTransaction && linkInfo[nodeId].lastKnownCommand != CMD_RELOCATE)) {
        EV_DETAIL << "We are either not in transaction with this node, or last command wasn't RELOCATE" << endl;
        return -EINVAL;
    }

//    cellVector scheduledCells  = linkInfo[nodeId].scheduledCells;
    cellVector relocationCells = linkInfo[nodeId].relocationCells;

//    EV_DETAIL << "Relocating cells:""\n newCells: " << newCells << "\nscheduledCells:\n"
//            << scheduledCells << "\nrelocationCells: " << relocationCells << endl;

    /* remove the first n cells that were nominated for relocation */
    for (int i = 0; i < (int) newCells.size(); ++i) {
        linkInfo[nodeId].scheduledCells.erase(std::find(
                linkInfo[nodeId].scheduledCells.begin(), linkInfo[nodeId].scheduledCells.end(), relocationCells[i]));
    }

//    EV_DETAIL << "scheduledCells after erasing " << newCells.size() << " relocationCells: " << scheduledCells << endl;

    /* add the new cells to our schedule */
    addCells(nodeId, newCells, linkOption);
    /* reset the record of cells nominated for relocation */
    linkInfo[nodeId].relocationCells.clear();

    return 0;
}

uint8_t TschLinkInfo::getLastKnownSeqNum(uint64_t nodeId) {
    Enter_Method_Silent();

    if (linkInfoExists(nodeId)) {
        return linkInfo[nodeId].lastKnownSeqNum;
    }
    return 0;
}

uint8_t TschLinkInfo::getSeqNum(uint64_t nodeId) {
    Enter_Method_Silent();

    if (linkInfoExists(nodeId)) {
        return linkInfo[nodeId].lastKnownSeqNum;
    }

    return 0;
}

void TschLinkInfo::incrementSeqNum(uint64_t nodeId) {
    Enter_Method_Silent();

    if (linkInfoExists(nodeId)) {
        if (linkInfo[nodeId].lastKnownSeqNum == 0xFF) {
            linkInfo[nodeId].lastKnownSeqNum = 1;
        } else {
            linkInfo[nodeId].lastKnownSeqNum++;
        }
    }
}

void TschLinkInfo::resetSeqNum(uint64_t nodeId) {
    Enter_Method_Silent();

    if (linkInfoExists(nodeId)) {
        linkInfo[nodeId].lastKnownSeqNum = 0;
    }
}

int TschLinkInfo::setLastKnownType(uint64_t nodeId, tsch6pMsg_t type) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId)) {
        return -EINVAL;
    }

    linkInfo[nodeId].lastKnownType = type;
    return 0;
}

tsch6pMsg_t TschLinkInfo::getLastKnownType(uint64_t nodeId) {
    Enter_Method_Silent();

    if (linkInfoExists(nodeId)) {
        return linkInfo[nodeId].lastKnownType;
    }
    return MSG_CONFIRMATION;
}

int TschLinkInfo::setLastKnownCommand(uint64_t nodeId, tsch6pCmd_t cmd) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId)) {
        return -EINVAL;
    }

    linkInfo[nodeId].lastKnownCommand = cmd;
    return 0;
}

tsch6pCmd_t TschLinkInfo::getLastKnownCommand(uint64_t nodeId) {
    Enter_Method_Silent();

    if (linkInfoExists(nodeId)) {
        return linkInfo[nodeId].lastKnownCommand;
    }
    return CMD_CLEAR;
}

int TschLinkInfo::setLastLinkOption(uint64_t nodeId, uint8_t linkOption) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId)) {
        return -EINVAL;
    }

    linkInfo[nodeId].lastLinkOption = linkOption;
    return 0;
}

uint8_t TschLinkInfo::getLastLinkOption(uint64_t nodeId) {
    Enter_Method_Silent();
    if (!linkInfoExists(nodeId)) {
        return MAC_LINKOPTIONS_NONE;
    }

    return linkInfo[nodeId].lastLinkOption;
}

void TschLinkInfo::startTimeoutTimer(uint64_t nodeId, simtime_t timeout) {
    if (!linkInfoExists(nodeId)) {
        EV_WARN << "Tried scheduling timeout timer, but no link info found for " << inet::MacAddress(nodeId) << endl;
        return;
    }

    if (!linkInfo[nodeId].tom) {
        linkInfo[nodeId].tom = new tschLinkInfoTimeoutMsg();
        linkInfo[nodeId].tom->setNodeId(nodeId);
    }

    linkInfo[nodeId].tom->setSeqNum(linkInfo[nodeId].lastKnownSeqNum);
    if (linkInfo[nodeId].tom->isScheduled()) {
        EV_WARN << "tschLinkInfoTimeoutMsg still scheduled, unsure if bug?" << endl;
        cancelEvent(linkInfo[nodeId].tom);
    }

    EV_DETAIL << "Scheduling timeout msg at " << timeout <<  "s for transaction to " << inet::MacAddress(nodeId) << endl;

    scheduleAt(timeout, linkInfo[nodeId].tom);
}

void TschLinkInfo::handleMessage(cMessage *msg) {
    tschLinkInfoTimeoutMsg* tom = dynamic_cast<tschLinkInfoTimeoutMsg*> (msg);

    if (tom && msg->isSelfMessage()) {
        EV_DETAIL << "TschLinkInfo received timeout msg" << endl;
        /* Abort transaction due to timeout */
//        auto type = to_string(linkInfo[tom->getNodeId()].lastKnownType);
//        auto cmd = to_string(linkInfo[tom->getNodeId()].lastKnownCommand);
//        auto seqNum = linkInfo[tom->getNodeId()].lastKnownSeqNum;

//        auto wstr = tsch::string_format("%s %s, seqNum %d with %s has timed out", type, cmd, seqNum, inet::MacAddress(tom->getNodeId()).str());

//        const char* w = (" with " + inet::MacAddress(tom->getNodeId()).str() + " timed out").c_str();
        //opp_warning(w);
//        EV_WARN << wstr << endl;
        /* Forward msg to sublayer so it can react properly */
        send(tom->dup(), sublayerControlOut);

        abortTransaction(tom->getNodeId());
    }
    else
        delete msg;
}

bool TschLinkInfo::matchingTimeOffset(std::tuple<cellLocation_t, uint8_t> const& obj,
                                      offset_t timeOffset) {
    cellLocation_t cell = std::get<0>(obj);
    return cell.timeOffset == timeOffset;
}
