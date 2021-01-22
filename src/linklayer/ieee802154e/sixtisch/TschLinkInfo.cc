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

    linkInfo[nodeId] = {
        .nodeId = nodeId,
        .inTransaction = inTransaction,
        .tom = new tschLinkInfoTimeoutMsg(),
        .lastKnownSeqNum = lastKnownSeqNum
    };

    linkInfo[nodeId].tom->setNodeId(nodeId);

    if (inTransaction) {
        startTimeoutTimer(nodeId, transactionTimeout);
    }

    return 0;
}

void TschLinkInfo::resetLink(uint64_t nodeId, tsch6pMsg_t lastKnownType) {
    abortTransaction(nodeId);
    clearCells(nodeId);
    resetSeqNum(nodeId);
    setLastKnownType(nodeId, lastKnownType);
    setLastLinkOption(nodeId, MAC_LINKOPTIONS_NONE);
}

void TschLinkInfo::revertLink(uint64_t nodeId, tsch6pMsg_t lastKnownType) {
    abortTransaction(nodeId);
    setLastKnownType(nodeId, lastKnownType);
    setLastLinkOption(nodeId, MAC_LINKOPTIONS_NONE);
}

bool TschLinkInfo::inTransaction(uint64_t nodeId) {
    Enter_Method_Silent();

    if (linkInfoExists(nodeId))
        return linkInfo[nodeId].inTransaction;

    return false;
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

int TschLinkInfo::addCell(uint64_t nodeId, cellLocation_t cell,
                            uint8_t linkOption) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId)) {
        return -EINVAL;
    }

    std::tuple<cellLocation_t, uint8_t> cellTuple = std::make_tuple(cell, linkOption);
    linkInfo[nodeId].scheduledCells.push_back(cellTuple);

    return 0;
}

int TschLinkInfo::addCells(uint64_t nodeId, const std::vector<cellLocation_t> &cellList, uint8_t linkOption)
{
    Enter_Method_Silent();

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
    for(auto & info: linkInfo) {
        auto it = std::find_if(info.second.scheduledCells.begin(), info.second.scheduledCells.end(),
                       [candidate](const std::tuple<cellLocation_t, uint8_t> & t) -> bool {
                         return std::get<0>(t) == candidate; });
        if (it != info.second.scheduledCells.end()) {
            return info.first;
        }
    }

    return 0;
}

void TschLinkInfo::clearCells(uint64_t nodeId) {
    Enter_Method_Silent();
    EV_INFO << "Clearing cells scheduled with " << inet::MacAddress(nodeId) << endl;

    if (linkInfoExists(nodeId))
        linkInfo[nodeId].scheduledCells.erase(
            std::remove_if( linkInfo[nodeId].scheduledCells.begin(), linkInfo[nodeId].scheduledCells.end(),
                [] (decltype(linkInfo[nodeId].scheduledCells)::value_type link) -> bool {
                    return !getCellOptions_isAUTO(std::get<1>(link));
                }),
            linkInfo[nodeId].scheduledCells.end()
        );
    else
        EV_WARN << "Instructed to clear but no linkInfo exists" << endl;
}

void TschLinkInfo::deleteCells(uint64_t nodeId, const std::vector<cellLocation_t> &cellList, uint8_t linkOption) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId)) {
        EV_WARN << "Instructed to delete cells with " << inet::MacAddress(nodeId) << " but no linkInfo found" << endl;
        return;
    }

    cellVector *scheduledCells = &(linkInfo[nodeId].scheduledCells); // TODO: replace with a proper getter method
    EV_INFO << "Deleting cells scheduled with " << inet::MacAddress(nodeId) << ": " << *scheduledCells << endl;
    auto it = cellList.begin();
    for(; (it != cellList.end()); ++it) {

        auto elem = std::find_if(scheduledCells->begin(), scheduledCells->end(),
                        [it](std::tuple<cellLocation_t, uint8_t> a) -> bool { return std::get<0>(a) == *it; });

        if (elem != scheduledCells->end())
            scheduledCells->erase(elem);
        else
            EV_WARN << "Instructed to delete cell " << *it << " but not found" << endl;
    }
    EV_INFO << endl;
}

bool TschLinkInfo::timeOffsetScheduled(offset_t timeOffset) {
    Enter_Method_Silent();

    /* loop through all links */
    for(auto & info: linkInfo) {
        auto it = std::find_if(info.second.scheduledCells.begin(), info.second.scheduledCells.end(),
                          [timeOffset](const std::tuple<cellLocation_t, uint8_t> & t) -> bool {
                            cellLocation_t cell = std::get<0>(t);
                            return cell.timeOffset == timeOffset; });
        if (it != info.second.scheduledCells.end()) {
            return true;
        }
    }

    return false;
}

bool TschLinkInfo::cellsInSchedule(uint64_t nodeId,
                                   std::vector<cellLocation_t> &cellList,
                                   uint8_t linkOption)
{
    Enter_Method_Silent();

    bool inSchedule = false;

    if (linkInfo.find(nodeId) != linkInfo.end()) {
        inSchedule = true;
        cellVector scheduled = linkInfo[nodeId].scheduledCells;
        std::vector<cellLocation_t>::iterator it; // TODO: replace with auto declaration
        for(it = cellList.begin(); it != cellList.end() && inSchedule; ++it) {
            auto currCell = std::make_tuple(*it, linkOption);
            inSchedule = std::find(scheduled.begin(), scheduled.end(), currCell) != scheduled.end();
        }
    }

    return inSchedule;
}

int TschLinkInfo::setRelocationCells(uint64_t nodeId,
                                     std::vector<cellLocation_t> &cellList,
                                     uint8_t linkOption) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId)) {
        return -EINVAL;
    }
    if ((linkInfo[nodeId].relocationCells.size() != 0) ||
        (linkInfo[nodeId].inTransaction == true)) {
        return -EINVAL;
    }

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

int TschLinkInfo::relocateCells(uint64_t nodeId, std::vector<cellLocation_t> &newCells,
                                uint8_t linkOption) {
    Enter_Method_Silent();

    if (!linkInfoExists(nodeId))
        return -EINVAL;

    if (!linkInfo[nodeId].inTransaction || (linkInfo[nodeId].inTransaction && linkInfo[nodeId].lastKnownCommand != CMD_RELOCATE))
        return -EINVAL;

    cellVector scheduledCells  = linkInfo[nodeId].scheduledCells;
    cellVector relocationCells = linkInfo[nodeId].relocationCells;
    /* remove the first n cells that were nominated for relocation */
    for(int i = 0; i < (int)newCells.size(); ++i) {
        scheduledCells.erase(std::find(scheduledCells.begin(),
                             scheduledCells.end(), relocationCells[i]));
    }
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
    if (!linkInfoExists(nodeId))
        return;

    linkInfo[nodeId].tom->setSeqNum(linkInfo[nodeId].lastKnownSeqNum);
    if (linkInfo[nodeId].tom->isScheduled()) {
        EV_WARN << "tschLinkInfoTimeoutMsg still scheduled, unsure if bug?" << endl;
        cancelEvent(linkInfo[nodeId].tom);
    }
    scheduleAt(timeout, linkInfo[nodeId].tom);
}

void TschLinkInfo::handleMessage(cMessage *msg) {
    tschLinkInfoTimeoutMsg* tom = dynamic_cast<tschLinkInfoTimeoutMsg*> (msg);

    if (tom && msg->isSelfMessage()) {
        /* abort transaction due to timeout */
//        auto type = to_string(linkInfo[tom->getNodeId()].lastKnownType);
//        auto cmd = to_string(linkInfo[tom->getNodeId()].lastKnownCommand);
//        auto seqNum = linkInfo[tom->getNodeId()].lastKnownSeqNum;

//        auto wstr = tsch::string_format("%s %s, seqNum %d with %s has timed out", type, cmd, seqNum, inet::MacAddress(tom->getNodeId()).str());

//        const char* w = (" with " + inet::MacAddress(tom->getNodeId()).str() + " timed out").c_str();
        //opp_warning(w);
//        EV_WARN << wstr << endl;
        /* forward msg to sublayer so it can react properly */
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
