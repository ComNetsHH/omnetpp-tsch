/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 * (minimal) implementation of the 6TiSCH 6top Scheduling Function
 * Zero / Experimental (SFX):
 * https://tools.ietf.org/html/draft-ietf-6tisch-6top-sfx
 *
 * Copyright (C) 2019  Institute of Communication Networks (ComNets),
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

#include "TschSFX.h"
#include <stdlib.h>
#include <numeric>
#include "inet/networklayer/common/InterfaceTable.h"
#include "../Ieee802154eMac.h"
#include "Tsch6topSublayer.h"

using namespace tsch;
using namespace inet;


TschSFX::TschSFX(uint64_t nodeId, TschLinkInfo *linkInfo, int slotframeSize,
                 int numChannels, Tsch6PInterface& tsch6p): pTsch6p(tsch6p) {
    pNodeId = nodeId;
    pTschLinkInfo = linkInfo;
    pSlotframeSize = slotframeSize;
    pNumChannels = numChannels;

    /* initialize random seed (perfectly fine if it's always the same for this
       node in our simulations) */
    srand(42+pNodeId);

    for (int i = 0; i < pSlotframeSize; ++i) {
        pWhitelist[i] = true;
    }

    /* TODO: what about fixed shared cells? set these! */
}

TschSFX::~TschSFX() {
}

void TschSFX::start() {
    /* send CLEAR command to all neighbors (as mandated by the draft) so that we
           can start fresh in case they still have unfinished transactions with us */
    auto mac = dynamic_cast<Ieee802154eMac*>(getParentModule()->getParentModule()->getParentModule()->getSubmodule("mac", 0));
    std::list<uint64_t> neighbors = dynamic_cast<Tsch6topSublayer*>(getParentModule()->getSubmodule("sixtop", 0))->getNeighborsInRange(pNodeId,mac);
    std::list<uint64_t>::iterator it;
    for(it = neighbors.begin(); it != neighbors.end(); ++it) {
        uint64_t neighborId = *it;
        pTsch6p.sendClearRequest(neighborId, pTimeout);
    }

    /* Send add requests for SFXTHRESH cells to all of our neighbors,
       as mandated by the draft */
    for(it = neighbors.begin(); it != neighbors.end(); ++it) {
        std::vector<cellLocation_t> cellList;
        uint64_t neighborId = *it;
        createCellList(neighborId, cellList, SFXTHRESH);
        pTsch6p.sendAddRequest(neighborId, MAC_LINKOPTIONS_TX, SFXTHRESH, cellList, pTimeout);
    }
}

tsch6pSFID_t TschSFX::getSFID() {
    return pSFID;
}

int TschSFX::createCellList(uint64_t destId, std::vector<cellLocation_t> &cellList,
                            int numCells) {
    if (cellList.size() != 0) {
        return -EINVAL;
    }

    int numCellsChecked = 0;
    int cellsChecked[pSlotframeSize];
    memset(cellsChecked, 0, pSlotframeSize * sizeof(int));

    while (numCellsChecked < (int) pWhitelist.size() &&
           (int)cellList.size() < numCells) {

        unsigned int candidateTimeOffset = rand() % pSlotframeSize;
        if (cellsChecked[candidateTimeOffset] == 0) {
            /* we haven't checked if this cell is available yet */
            if (pWhitelist[candidateTimeOffset]) {
                /* cell is unused, can be assigned */
                /* pick channelOffset between 1 and pNumChannels */
                unsigned int channelOffset = (rand() % (pNumChannels-1)) + 1;
                cellList.push_back({candidateTimeOffset, channelOffset});
                /* since we're "promising" this cell to another node, it can't
                   be used anymore */
                pWhitelist[candidateTimeOffset] = false;
            }

            cellsChecked[candidateTimeOffset] = 1;
            numCellsChecked++;
        }
    }

    if (cellList.size() == 0) {
        return -ENOSPC;
    }
    if ((int) cellList.size() < numCells) {
        return -EFBIG;
    }

    return 0;
}

int TschSFX::pickCells(uint64_t destId, std::vector<cellLocation_t> &cellList,
                       int numCells, bool isRX, bool isTX, bool isSHARED) {
    if (cellList.size() == 0) {
        return -EINVAL;
    }

    std::vector<cellLocation_t>::iterator it = cellList.begin();
    int numCellsPicked = 0;
    for(; it != cellList.end() && numCellsPicked < numCells; ++it) {
        unsigned int timeOffset = it->timeOffset;
        if (pWhitelist[timeOffset]) {
            /* cell is still available. pick that. */
            pWhitelist[timeOffset] = false;
            numCellsPicked++;
        } else {
            /* we can't use that cell, remove it from cellList */
            cellList.erase(it, it);
        }
    }

    if (numCellsPicked == 0) {
        return -ENOSPC;
    }
    if (numCellsPicked < numCells) {
        return -EFBIG;
    }

    return 0;
}

void TschSFX::handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells,
                             std::vector<cellLocation_t> *cellList) {
    // TODO implement me!
}

void TschSFX::handleInconsistency(uint64_t destId, uint8_t seqNum) {
    // TODO: implement me! (note that the draft doesn't specify how to do this yet >.<)
}

int TschSFX::getTimeout() {
    return pTimeout;
}

int TschSFX::updateCells(uint64_t destId) {
    int numCells = 0;
    int requiredCells = estimateRequiredCells(destId);
    if (requiredCells > 0 ) {
        numCells = allocationPolicy(destId, numCells);
    }

    if (numCells < 0 ) {
        // TODO send delete request here
        // TODO impl pickCells for deletion!
        // TODO erase pdrInfo after we've successfully deleted (or relocated) a cell!
    }
    if (numCells > 0) {
        std::vector<cellLocation_t> cellList;
        pickCells(destId, cellList, numCells, false, true, false);
        pTsch6p.sendAddRequest(destId, MAC_LINKOPTIONS_TX, numCells, cellList, pTimeout);
    }

    return 0;
}

void TschSFX::recordPDR(cMessage* msg) {
//    /* TODO: make sure we only record PDR for softcells (i.e. not for the fixed
//       cells that cannot be rescheduled) */
//    uint64_t sender = msg->getNodeId();
//    /* store success as int for easy insertion into pdrInfo */
//    int success = msg->getTxSuccess() ? 1 : 0;
//    cellLocation_t cell = msg->getCell();
//
//    if(pdrInfo.find(cell) != pdrInfo.end()) {
//        /* no info on this cell yet, start collecting */
//        std::vector<int> vec;
//        pdrInfo[cell] = vec;
//    }
//
//    /* record our new pdr info */
//    pdrInfo[cell].push_back(success);
//
//    if (pdrInfo[cell].size() == 11) {
//        /* we have a "fully grown" tx success record; remove the oldest element
//           to shrink it back into size */
//        pdrInfo[cell].erase(pdrInfo[cell].begin());
//
//        /* check if PDR falls below PDR_THRESHOLD, trigger relocation */
//        /* NOTE: this has no hysteresis and doesn't accumulate broken cells. Extremely
//           naive implementation! (but standard-compliant.. :D)*/
//        int newPdrStat = std::accumulate(pdrInfo[cell].begin(), pdrInfo[cell].end(), 0);
//        if (newPdrStat < PDR_THRESHOLD) {
//            int numCells = 1;
//            std::vector<cellLocation_t> relocationCellList;
//            std::vector<cellLocation_t> candidateCellList;
//            relocationCellList.push_back(cell);
//            pickCells(sender, candidateCellList, numCells, false, true, false);
//            pTsch6p.sendRelocationRequest(sender, MAC_LINKOPTIONS_TX, numCells,
//                                          relocationCellList, candidateCellList,
//                                          pTimeout);
//        }
//    }
}


int TschSFX::estimateRequiredCells(uint64_t destId) {
    int numUsedCells = pTschLinkInfo->getNumCells(destId);
    int requiredCells = numUsedCells + OVERPROVISION;

    return requiredCells;
}

int TschSFX::allocationPolicy(uint64_t destId, int requiredCells) {
    int scheduledCells = pTschLinkInfo->getNumCells(destId);
    int numCells = 0;

    if (requiredCells < (scheduledCells - SFXTHRESH) ||
        (scheduledCells < requiredCells)) {
        /* we need to delete or add cells. (If the first condition is true,
           numCells will be a negative number, indicating a delete.)*/
        numCells = requiredCells - scheduledCells;
    }

    return numCells;
}
