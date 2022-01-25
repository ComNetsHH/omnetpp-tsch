/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 * (minimal) implementation of the 6TiSCH 6top Scheduling Function
 * Zero / Experimental (SFX):
 * https://tools.ietf.org/html/draft-ietf-6tisch-6top-sfx
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

#ifndef __WAIC_TSCHSFX_H_
#define __WAIC_TSCHSFX_H_

#include <omnetpp.h>

#include "../Tsch6pInterface.h"
#include "../TschLinkInfo.h"
#include "../TschSF.h"

class TschSFX: public TschSF {
public:
    TschSFX(uint64_t nodeId, TschLinkInfo *linkInfo, int slotframeSize,
            int numChannels, Tsch6PInterface& tsch6p);
    ~TschSFX();

    /**
     * @brief Start operating. This function MUST be called after creating a SF
     *        object, but not before all other nodes are initialized!
     *        (otherwise this SF object will be sending while other nodes are
     *        still unable to react correctly)
     */
    virtual void start();

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
     *                       -EINVAL if cellList is not empty
     */
    virtual int createCellList(uint64_t destId, std::vector<cellLocation_t> &cellList,
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
     *                       -EINVAL if cellList is empty
     */
    virtual int pickCells(uint64_t destId, std::vector<cellLocation_t> &cellList,
                          int numCells, bool isRX, bool isTX, bool isSHARED);

    /**
     * @brief React to a 6P response.
     *
     * @param sender         The node that originated the response
     * @param code           The return code of the response to be handled
     * @param numCells       The value of the response's numCells field (if any)
     * @param cellList       The cellList contained in the response (if any)
     */
    virtual void handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells = -1,
                                std::vector<cellLocation_t> *cellList = NULL);

    void handleTransactionTimeout(uint64_t sender) {}
    void freeReservedCellsWith(uint64_t nodeId) {}
    void handle6pClearReq(uint64_t nodeId) {}

    /**
     * @brief Handle the inconsistency which was uncovered by @p seqNum
     *        in the schedule maintained with @p destId
     */
    virtual void handleInconsistency(uint64_t destId, uint8_t seqNum);

    /**
     * @return The 6P Timeout value defined by this Scheduling Function
     */
    virtual int getTimeout();

    /**
     * @brief Add or delete cells to/from the link to @p destId. This method
     *        determines whether and how many cells to add or delete using the
     *        cell estimation and allocation algorithms defined by SFX.
     *
     * @return               0  on success,
     *                       -EINVAL otherwise
     */
    int updateCells(uint64_t destId);

    /**
     * @brief Update the PDR information for the link to the node specified in
     *        @p msg with the information from @p msg. The PDR is calculated as
     *        mandated by draft-ietf-6tisch-6top-sfx-00:
     *        "Packet Delivery Rate (PDR) is calculated per cell, as the
     *        percentage of acknowledged packets, for the last 10 packet
     *        transmission attempts."
     */
    void recordPDR(cMessage* msg);

    virtual void incrementNeighborCellElapsed(uint64_t neighborId) override {}
    virtual void decrementNeighborCellElapsed(uint64_t neighborId) override {}

private:
    /** Thresholds for cell over-provisioning.
     TODO: these numbers are picked arbitrarily for now, play around with it!
     TODO: make them settable in .ini file? */
    const int SFXTHRESH = 5;
    const int OVERPROVISION = 5;
    const int PDR_THRESHOLD = 7;

    // TODO pick value! (pulled this one out of my nose)
    const int pTimeout = 1000;

    /** The Identifier of this Scheduling Function*/
    const tsch6pSFID_t pSFID = SFID_SFX;

    /* the ID of this node, derived from its MAC address */
    uint64_t pNodeId;

    int pSlotframeSize;
    int pNumChannels;

    /** Information about all links maintained by this node */
    TschLinkInfo *pTschLinkInfo;

    /** The 6P instance this SF is a part of. */
    Tsch6PInterface &pTsch6p;

    /**
     * Packet Delivery Ration information about all cells currently in use.
     * The vector contains information on the last 10 packet transmission attempts:
     * value '1' for each successful attempt, value '0' for each failure.
     * If < 10 attempts have been made to date, the vector contains < 10 values.
     */
    std::map<cellLocation_t, std::vector<int>> pdrInfo;

    /** Tracks whether a cell is free to be assigned or occupied. Each key
     *  corresponds to a timeOffset. If the value is true, the cell is free. If
     *  it is false, the cell is occupied and cannot be assigned. */
    std::map<unsigned int, bool> pWhitelist;

    /**
     * @brief The Cell Estimation Algorithm (CEA) as defined by the SFX draft,
     *        which determines how many cells should be scheduled to @p destId.
     *
     * @param destId         The neighbor for which to make the calculations
     * @return               The number of cells
     *
     */
    int estimateRequiredCells(uint64_t destId);

    /**
     * @brief The allocation policy as defined by the SFX draft:
     *        Decides whether cells need to be added/deleted in order to reach
     *        @p requiredCells.
     *
     * @return               The number of cells to be added or deleted.
     *                       If cells are to be deleted, the return value is negative.
     *                       If cells are to be added, the return value is positive.
     */
    int allocationPolicy(uint64_t destId, int requiredCells);
};

#endif /*__WAIC_TSCHSFX_H_*/
