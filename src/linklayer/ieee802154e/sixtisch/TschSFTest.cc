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