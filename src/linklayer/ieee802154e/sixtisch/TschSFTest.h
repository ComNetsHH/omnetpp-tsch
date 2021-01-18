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

#ifndef __WAIC_TSCHSFTEST_H_
#define __WAIC_TSCHSFTEST_H_

#include <omnetpp.h>

#include "TschLinkInfo.h"
#include "TschSF.h"

class TschSFTest: public TschSF {
public:
    TschSFTest(TschLinkInfo *linkInfo);
    ~TschSFTest();

    /**
     * @return    the Scheduling Function Identifier (SFID) of this SF.
     */
    virtual tsch6pSFID_t getSFID();

    /**
     * @brief Create a cellList to be proposed to the neighbor at @p destId (i.e.
     *        when creating an ADD or RELOCATE request). @p numCells specifies
     *        the desired number of cells at the end of the transaction, so @p
     *        cellList will contain at least @p numCells entries if enough cells
     *        are available. If not enough, bit >0 cells are available,
     *        they will be added to @p cellList nonetheless, but this
     *        will be indicated in the return value.
     *
     * @param destId         The neighbor to propose the cellList to
     * @param[out] cellList  The vector into which the cellList will be written
     * @param numCells       The minimum number of cells requested
     * @return               0 on success
     *                       -EFBIG  on partial success, i.e. if some, but less than
     *                               @p numCells cells were added to @p cellList
     *                       -ENOSPC on failure, i.e. if no suitable cell could be found
     *                       -EINVAL if cellList is not empty or destId is unknown
     */
    virtual int createCellList(int destId, std::vector<cellLocation_t> &cellList,
                               int numCells);

    /**
     * @brief Pick cells from cellList that was proposed by the neighbor at @p destId
     *        (i.e. in an ADD or RELOCATE request). @p numCells specifies
     *        the desired number of cells at the end of the transaction, so @p
     *        cellList will contain at least @p numCells entries if enough cells
     *        are available. If not enough, but >0 cells are available,
     *        they will be added to @p cellList nonetheless, but this
     *        will be indicated in the return value.
     *
     * @param destId         The neighbor which proposed the cellList
     * @param[in,out] cellList  The vector containing the proposed cells. On return,
     *                       it will contain the cells picked by this function.
     * @param numCells       The minimum number of cells requested
     * @param isRX           Must be true if cellList should include RX cells
     * @param isTX           Must be true if cellList should include TX cells
     * @param isSHARED       Must be true if cellList should include SHARED cells
     * @return               0 on success
     *                       -EFBIG  on partial success, i.e. if some, but less than
     *                               @p numCells cells were picked for @p cellList
     *                       -ENOSPC on failure, i.e. if no suitable cell could be found
     *                       -EINVAL if cellList is empty or destId is unknown
     */
    virtual int pickCells(int destId, std::vector<cellLocation_t> &cellList,
                          int numCells, bool isRX, bool isTX, bool isSHARED);

    /**
     * @brief Handle the inconsistency which was uncovered by @p seqNum
     *        in the schedule maintained with @p destId
     */
    virtual void handleInconsistency(int destId, uint8_t seqNum);

    /**
     * @return The 6P Timeout value defined by this Scheduling Function
     */
    virtual int getTimeout();

private:
    /** The Identifier of this Scheduling Function*/
    const tsch6pSFID_t pSFID = SFID_TEST;

    /** Information about all links maintained by this node */
    TschLinkInfo *pTschLinkInfo;
};

#endif /*__WAIC_TSCHSFTEST_H_*/
