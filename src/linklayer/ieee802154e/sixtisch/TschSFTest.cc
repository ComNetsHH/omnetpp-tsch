/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 * Test SF to quickly check whether 6P works as expected
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

#include "TschSFTest.h"

TschSFTest::TschSFTest(TschLinkInfo *linkInfo) {
    pTschLinkInfo = linkInfo;
}

TschSFTest::~TschSFTest() {
}

tsch6pSFID_t TschSFTest::getSFID() {
    return pSFID;
}

int TschSFTest::createCellList(int destId, std::vector<cellLocation_t> &cellList,
                               int numCells) {
    /* The purpose of this cellList is to make both nodes of the link operate on
       different channels than the default hopping pattern at all times in order
       to check whether the channel switch implementation works correctly (if
       they can only communicate with each other after the successful ADD, it
       worked.) */

    // TODO: erst mal an hopping sequence length angepasst, mal gucken wie das
    // dann tatsächlich in der Impl. ausschaut
    int slotframeSize = 16;

    for(unsigned int timeOffset = 0; timeOffset < slotframeSize; ++timeOffset) {
        // always operate 1 channel away from the default hopping sequence
        cellList.push_back({timeOffset, 1});
    }

    return 0;
}

int TschSFTest::pickCells(int destId, std::vector<cellLocation_t> &cellList,
                          int numCells, bool isRX, bool isTX, bool isSHARED) {
    // only accept cells from node 1
    if (destId == 1) {
    // just blindly accept what we've been proposed
        return 0;
    }

    return -ENOSPC;
}

void TschSFTest::handleInconsistency(int destId, uint8_t seqNum) {
    // TODO implement me!
}

int TschSFTest::getTimeout() {
    return 5000;
}
