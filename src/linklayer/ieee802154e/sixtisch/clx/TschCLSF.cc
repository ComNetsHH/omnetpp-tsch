/*
 * Cross-layer Scheduling Function.
 *
 * Copyright (C) 2021  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2021  Yevhenii Shudrenko
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

#include "TschCLSF.h"
#include "TschSfControlInfo.h"

using namespace tsch;

Define_Module(TschCLSF);

TschCLSF::TschCLSF() :
    crossLayerChOffset(-1),
    crossLayerSlotRange({-1, -1}),
    num6pRelocateAttempts(0),
    maxNum6pAttempts(3), // FIXME: initialize from .ini parameter
    isDaisyChained(false),
    isDaisyChainFailed(false),
    slotframeChunkPad(3)
{}


TschCLSF::~TschCLSF() {}

void TschCLSF::initialize(int stage) {
    TschMSF::initialize(stage);
    if (stage == 6) {
        rpl->subscribe("reschedule", this);
        rpl->subscribe("setChOffset", this);
        WATCH(crossLayerChOffset);
        WATCH(crossLayerSlotRange);
        WATCH(num6pRelocateAttempts);
    }
}

void TschCLSF::refreshDisplay() const {
    if (!rplParentId)
        return;

    std::ostringstream out;
    std::vector<cellLocation_t> txCells = pTschLinkInfo->getDedicatedCells(rplParentId);

    std::sort(txCells.begin(), txCells.end(),
                [](const cellLocation_t c1, const cellLocation_t c2) { return c1.timeOffset < c2.timeOffset; });
    out << txCells;

    if (isDaisyChained)
        hostNode->getDisplayString().setTagArg("t", 2, "#28b52d");
    if (isDaisyChainFailed)
        hostNode->getDisplayString().setTagArg("t", 2, "#ff0000");

    hostNode->getDisplayString().setTagArg("t", 0, out.str().c_str());
}

offset_t TschCLSF::chooseCrossLayerChOffset() {
    // FIXME: Magic numbers
    int start = crossLayerChOffset - 2;
    int end = crossLayerChOffset + 2;

    return (offset_t) intuniform(start >= 0 ? start : 0, end <= pNumChannels - 1 ? end : pNumChannels - 1);
}

void TschCLSF::deleteCells(uint64_t nodeId, int numCells) {
    // Only delete cells if the daisy-chaining has not yet started
    if (!isCrossLayerInfoAvailable())
        TschMSF::deleteCells(nodeId, numCells);
}

void TschCLSF::handleSelfMessage(cMessage* msg) {
    EV_DETAIL << "CLSF handling self-msg - " << msg << endl;
    auto copy = msg->dup();

    TschMSF::handleSelfMessage(msg);
    if (copy->getKind() == CHECK_DAISY_CHAIN)
        checkDaisyChained();

    delete copy;
}

void TschCLSF::handleSuccessRelocate(uint64_t sender, std::vector<cellLocation_t> cellList) {
    if (sender != rplParentId) {
        EV_WARN << "Received RELOCATE response from node other than preferred parent!" << endl;
        return;
    }

    if (!isCrossLayerInfoAvailable()) {
        EV_WARN << "Received RELOCATE response although no CLX info is available?!" << endl;
        return;
    }

    if (!cellList.size()) {
        EV_WARN << "Received empty RELOCATE response during daisy-chaining" << endl;

        if (num6pRelocateAttempts > maxNum6pAttempts) {
            EV_DETAIL << "Maximum number RELOCATEs attempted, daisy-chaining is considered failed" << endl;
            isDaisyChainFailed = true;
            return;
        }

        auto dedicatedCells = pTschLinkInfo->getDedicatedCells(sender);

        if (!dedicatedCells.size()) {
            EV_DETAIL << "No dedicated cells found to relocate, daisy-chaining is considered finished" << endl;
            isDaisyChained = true;
            return;
        }

        num6pRelocateAttempts++;

        // Might break 6top transaction handling flow
//        auto selfMsg = new cMessage("SEND_6P_DELAYED", SEND_6P_REQ);
//        auto ctrlInfo = new SfControlInfo(sender);
//        ctrlInfo->set6pCmd(CMD_RELOCATE);
//        ctrlInfo->setCellList(dedicatedCells);
//        selfMsg->setControlInfo(ctrlInfo);
//
//        auto timeoutVal = pow(2, uniform(0, num6pRelocateAttempts));
//        scheduleAt(simTime() + timeoutVal, selfMsg);
//        EV_DETAIL << "Preparing to send 6P RELOCATE in " << timeoutVal
//                << " s, at " << simTime() + timeoutVal << endl;


        scheduleAt(simTime() + pow(2, intrand(num6pRelocateAttempts)), new cMessage("CHECK_DAISY_CHAIN", CHECK_DAISY_CHAIN));

//        relocateCells(sender, dedicatedCells);
    }
    else
    {
        // TODO: weak assumption that we don't relocate cells due to reasons other than for daisy-chaining
        EV_DETAIL << "Successfully relocated cells: " << cellList << endl;
        isDaisyChained = true;
    }
}

vector<offset_t> TschCLSF::getAvailableSlotsInRange(int start, int end, int pad) {
    // heuristic to loosen the bounds of a slotframe chunk a little
   start -= pad;
   end += pad;

   if (start < 0) {
       end += pad; // compensate for tight starting bound from the end
       start = 0;
   }

   if (end > pSlotframeLength) {
       end = pSlotframeLength;
       start -= pad; // compensate for tight closing bound from the start
   }

   EV_DETAIL << "CLSF looking for available slots in range: " << start << ", " << end << ", padding = " << pad << endl;

   auto freeSlots = TschMSF::getAvailableSlotsInRange(start, end);

   EV_DETAIL << "Found free slots: " << freeSlots << endl;

   return freeSlots;
}

int TschCLSF::createCellList(uint64_t destId, std::vector<cellLocation_t> &cellList, int numCells) {
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

    std::vector<offset_t> availableSlots;

    if (isCrossLayerInfoAvailable()) {
        availableSlots = TschCLSF::getAvailableSlotsInRange(crossLayerSlotRange.start, crossLayerSlotRange.end, slotframeChunkPad);

        if (!availableSlots.size()) {
            EV_DETAIL << "No free slots found in the assigned slotframe chunk, searching from 0th slot" << endl;

            availableSlots = TschMSF::getAvailableSlotsInRange(0, crossLayerSlotRange.start);

            if (!availableSlots.size()) {
                EV_DETAIL << "No free slots found from 0th slot, searching across the whole slotframe" << endl;

                availableSlots = TschMSF::getAvailableSlotsInRange(0, pSlotframeLength);
            }
        }
    }
    else
        availableSlots = TschMSF::getAvailableSlotsInRange(0, pSlotframeLength);

    if (!availableSlots.size()) {
        EV_DETAIL << "No available cells found" << endl;
        return -ENOSPC;
    }
    else
        EV_DETAIL << "Found availabe slot offsets: " << availableSlots << endl;

    if ((int) availableSlots.size() < numCells) {
        EV_DETAIL << "More cells requested than total available, returning "
                << availableSlots.size() << " cells" << endl;
        for (auto slOffset : availableSlots)
        {
            cellList.push_back({slOffset, crossLayerChOffset != -1 ? chooseCrossLayerChOffset() : (offset_t) intrand(pNumChannels)});
            reservedTimeOffsets[destId].push_back(slOffset);
        }

        return -EFBIG;
    }

    // Fill cell list with all available slot offsets and random channel offset
    for (auto sl : availableSlots)
        cellList.push_back({sl, crossLayerChOffset != -1 ? chooseCrossLayerChOffset()  : (offset_t) intrand(pNumChannels)});

    EV_DETAIL << "Initialized cell list: " << cellList << endl;

    // Select only required number of cells from cell list
    cellList = pickRandomly(cellList, numCells);
    EV_DETAIL << "After picking required number of cells (" << numCells << "): " << cellList << endl;

    // Block selected slot offsets until 6P transaction finishes
    for (auto c : cellList)
        reservedTimeOffsets[destId].push_back(c.timeOffset);

    return 0;
}

void TschCLSF::relocateCells(uint64_t neighborId, vector<cellLocation_t> relocCells) {
    EV_DETAIL << "CLSF version of relocateCells() invoked" << endl;
    if (!relocCells.size()) {
        EV_DETAIL << "No cells provided to relocate" << endl;
        return;
    }

    if (pTschLinkInfo->inTransaction(neighborId)) {
        EV_WARN << "Can't relocate cells, currently in another transaction with this node" << endl;
        return;
    }

    if (!isCrossLayerInfoAvailable()) {
        EV_WARN << "Trying to relocate cells without CLX info available!" << endl;
        return;
    }

    if (neighborId != rplParentId) {
        EV_WARN << "Trying to relocate cells with node other than preferred parent" << endl;
        return;
    }

    EV_DETAIL << "Relocating cell(s) with " << MacAddress(neighborId) << " : " << relocCells << endl;

    auto availableSlots = TschMSF::getAvailableSlotsInRange(crossLayerSlotRange.start, crossLayerSlotRange.end);

    if ((int) availableSlots.size() < (int) relocCells.size()) {
        EV_WARN << "Not enough free slot offsets to relocate currently scheduled cells" << endl;
        return;
    }

    std::vector<cellLocation_t> candidateCells = {};
    for (auto slof: availableSlots)
        candidateCells.push_back( {slof, chooseCrossLayerChOffset()} );

    if (availableSlots.size() > relocCells.size() + pCellListRedundancy)
        candidateCells = pickRandomly(candidateCells, relocCells.size() + pCellListRedundancy);

    EV_DETAIL << "Selected candidate cell list to accommodate relocated cells: " << candidateCells << endl;

    for (auto cc : candidateCells)
        reservedTimeOffsets[neighborId].push_back(cc.timeOffset);

    pTsch6p->sendRelocationRequest(neighborId, MAC_LINKOPTIONS_TX, relocCells.size(), relocCells, candidateCells, pTimeout);
}

void TschCLSF::receiveSignal(cComponent *src, simsignal_t id, long value, cObject *details)
{
    Enter_Method_Silent();

    TschMSF::receiveSignal(src, id, value, details);

    std::string signalName = getSignalName(id);

    // TODO: refactor into signal references
    if (std::strcmp(signalName.c_str(), "setChOffset") == 0)
    {
        setBranchChannelOffset((int) value);
    }
    else if (std::strcmp(signalName.c_str(), "reschedule") == 0)
    {
        auto slChunk = ((TschSfControlInfo*) details)->getSlotRange();

        handleDaisyChaining(slChunk);
    }
}

void TschCLSF::setBranchChannelOffset(int chOf) {
    if (crossLayerChOffset != -1) {
        EV_WARN << "Channel offset is already fixed, discarding signal" << endl;
        return;
    }

    if (chOf < 0) {
        EV_WARN << "Invalid channel offset advertised by RPL" << endl;
        return;
    }

    crossLayerChOffset = chOf;
    EV_DETAIL << "Received branch-specific channel offset: " << chOf << " from RPL" << endl;
}

void TschCLSF::handleSuccessAdd(uint64_t sender, int numCells,  vector<cellLocation_t> cellList) {

    TschMSF::handleSuccessAdd(sender, numCells, cellList);
    if (isCrossLayerInfoAvailable() && !isDaisyChained && !isDaisyChainFailed)
        checkDaisyChained();
}

void TschCLSF::handleDaisyChaining(SlotframeChunk advertisedChunk) {
    if (!isValidSlotframeChunk(advertisedChunk)) {
        EV_WARN << "Cannot proceed with daisy-chaining, invalid chunk slotframe chunk provided - " << advertisedChunk << endl;
        isDaisyChainFailed = true;
        return;
    }

    if (!rplParentId) {
        EV_WARN << "Cannot proceed with daisy-chaining, RPL parent not set!" << endl;
        isDaisyChainFailed = true;
        return;
    }

    if (crossLayerChOffset == -1) {
        EV_WARN << "Cannot proceed with daisy-chaining, branch-unique channel offset not set" << endl;
        isDaisyChainFailed = true;
        return;
    }

    crossLayerSlotRange.start = advertisedChunk.start;
    crossLayerSlotRange.end = advertisedChunk.end;

    checkDaisyChained();
}

void TschCLSF::resetStateWith(uint64_t nbrId) {
    TschMSF::resetStateWith(nbrId);
    checkDaisyChained();
}

void TschCLSF::checkDaisyChained()
{
    if (isDaisyChained) {
        EV_DETAIL << "Already daisy-chained" << endl;
        return;
    }

    if (!isCrossLayerInfoAvailable()) {
        EV_DETAIL << "No cross-layer info available" << endl;
        return;
    }

    auto dedicatedCells = pTschLinkInfo->getDedicatedCells(rplParentId);

    if (!dedicatedCells.size()) {
        EV_DETAIL << "No dedicated cells found to relocate, daisy-chaining finished" << endl;
        isDaisyChained = true;
        return;
    } else {
        auto nonDaisyChained = getNonDaisyChainedCells(dedicatedCells);

        if (!nonDaisyChained.size()) {
            EV_DETAIL << "No cells require relocation" << endl;
            isDaisyChained = true;
            return;
        }

        EV_DETAIL << "\n Attempting daisy-chaining, slotframe chunk: "
                << crossLayerSlotRange << " and channel offset = " << crossLayerChOffset << endl;

        // This may break 6P transaction handling process by 6top and TschLinkInfo
//        auto selfMsg = new cMessage("SEND_6P_DELAYED", SEND_6P_REQ);
//        auto ctrlInfo = new SfControlInfo(rplParentId);
//        ctrlInfo->set6pCmd(CMD_RELOCATE);
//        ctrlInfo->setCellList(nonDaisyChained);
//        selfMsg->setControlInfo(ctrlInfo);
//
//        auto delay = uniform(10, 20); // FIXME: magic numbers
//
//        scheduleAt(simTime() + delay, selfMsg);
//        EV_DETAIL << "Preparing to send 6P RELOCATE in " << delay << " s, at " << simTime() + delay << endl;

        relocateCells(rplParentId, nonDaisyChained);

        num6pRelocateAttempts++;

    }
}

vector<cellLocation_t> TschCLSF::getNonDaisyChainedCells(vector<cellLocation_t> cellList) {
    if (!isCrossLayerInfoAvailable()) {
        EV_WARN << "Cannot determine whether cells have to be relocated, no CL info available!" << endl;
        return cellList;
    }

    std::vector<cellLocation_t> requireRelocation = {};
    for (auto c : cellList) {
        if (c.timeOffset >= crossLayerSlotRange.start && c.timeOffset <= crossLayerSlotRange.end)
            continue;
        requireRelocation.push_back(c);
    }

    if (requireRelocation.size())
        EV_DETAIL << "Filtered cells that require relocation: " << requireRelocation << endl;

    return requireRelocation;
}





