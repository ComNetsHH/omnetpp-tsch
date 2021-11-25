/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 * Interface that must be implemented by each 6TiSCH
 * Scheduling Function.
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

#ifndef __WAIC_TSCHSF_H_
#define __WAIC_TSCHSF_H_

#include <vector>
#include <omnetpp.h>
#include "WaicCellComponents.h"

#include "Tsch6tischComponents.h"
//#include "tschSpectrumSensingResult_m.h"

using namespace omnetpp;

class TschSF: public cSimpleModule {
public:
    /**
     * @brief Start operating. This function MUST be called after creating a SF
     *        object, but not before all other nodes are initialized!
     *        (otherwise this SF object will be sending while other nodes are
     *        still unable to react correctly)
     */
    virtual void start() = 0;

    /**
     * @return    the Scheduling Function Identifier (SFID) of this SF.
     */
    virtual tsch6pSFID_t getSFID() = 0;

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
    virtual int createCellList(uint64_t destId, std::vector<cellLocation_t> &cellList,
                               int numCells) = 0;

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
    virtual int pickCells(uint64_t destId, std::vector<cellLocation_t> &cellList,
                          int numCells, bool isRX, bool isTX, bool isSHARED) = 0;

    /**
     * @brief React to a 6P response. While the actual reaction (updating
     *        schedules etc) is done by 6P, a Scheduling Function might want
     *        to adjust its internal state in reaction to a 6P response.
     *
     * @param sender         The node that originated the response
     * @param code           The return code of the response to be handled
     * @param numCells       The value of the response's numCells field (if any)
     * @param cellList       The cellList contained in the response (if any)
     */
    virtual void handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells = -1,
                                std::vector<cellLocation_t> *cellList = NULL) = 0;

    /**
     * @brief More "nullptr exceptions"-safe version of handleResponse().
     *
     * @overload
     */
    virtual void handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells = -1,
                                        std::vector<cellLocation_t> cellList = {}) = 0;

    /**
     * @brief Handle the inconsistency which was uncovered by @p seqnum
     *        in the schedule maintained with @p destId
     */
    virtual void handleInconsistency(uint64_t destId, uint8_t seqNum) = 0;

    virtual void handleTransactionTimeout(uint64_t sender) = 0;

    virtual void freeReservedCellsWith(uint64_t nodeId) = 0;

    /**
     * @return The 6P Timeout value defined by this Scheduling Function in ms
     */
    virtual int getTimeout() = 0;

     /**
     * @brief Update the PDR information for the link to the node specified in
     *        @p msg with the information from @p msg. The PDR is calculated as
     *        mandated by draft-ietf-6tisch-6top-sfx-00:
     *        "Packet Delivery Rate (PDR) is calculated per cell, as the
     *        percentage of acknowledged packets, for the last 10 packet
     *        transmission attempts."
     *
     *        This function is only needed by SFX, but part of the interface
     *        to escape circular dependency hell.
     */
    virtual void recordPDR(cMessage* msg) = 0;

    /**
     * @brief Handle an update from the @ref TschSpectrumSensing module.
     *        These updates will arrive after each completed spectrum sweep.
     *        Might trigger a cell relocation.
     *
     *        This function is only needed by SFSB, but part of the interface
     *        to escape circular dependency hell.
     *
     * @param ssr            The results of one sweep, i.e. fresh channel
     *                       quality information for each channel in use
     */
    //virtual void handleSpectrumSensingResult(tschSpectrumSensingResult* ssr) = 0;

    /**
     * @brief Handle @p data that was piggybacked by @p sender.
     *
     *        This function is only needed by SFSB, but part of the interface
     *        to escape circular dependency hell.
     *
     * @param sender         The node that sent the data
     * @param data           Pointer to the data that was piggybacked. Note that
     *                       the message only contained the pointer, NOT the data
     *                       (this works because this is not the real world ;)).
     *                       It is the task of the SF to explicitly free the data
     *                       behind the pointer once it's done processing it.
     */
    virtual void handlePiggybackedData(uint64_t sender, void* data) = 0;
};

#endif /*__WAIC_TSCHSF_H_*/
