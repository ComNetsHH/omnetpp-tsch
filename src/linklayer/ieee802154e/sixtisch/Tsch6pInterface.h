/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 * Interface for a 6P implementation. Used to enable Scheduling Function
 * implementations to send messages via the Tsch6topSublayer they are a part of.
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

#ifndef __WAIC_TSCH6PINTERFACE_H
#define __WAIC_TSCH6PINTERFACE_H

#include "WaicCellComponents.h"
#include "Tsch6tischComponents.h"

class Tsch6PInterface {
public:
    /**
     * @brief Send a 6P Add request.
     *
     * @param destId       Id of the node to which this request will be sent
     * @param cellOptions  The CellOptions as described in the 6P draft
     * @param numCells     The number of cells that are requested for allocation
     * @param cellList     Cells proposed for allocation (must have length
     *                     >= @param numCells)
     * @param timeout      Timespan in ms after which a response is
     *                     expected at the latest
     */
    virtual void sendAddRequest(uint64_t destId, uint8_t cellOptions, int numCells,
                        std::vector<cellLocation_t> &cellList, int timeout) = 0;

    /**
     * @brief Send a 6P Delete request.
     *
     * @param destId       Id of the node to which this request will be sent
     * @param cellOptions  The CellOptions as described in the 6P draft
     * @param numCells     The number of cells that are requested for deletion
     * @param cellList     Cells proposed for allocation (must have length
     *                     >= @param numCells)
     * @param timeout      Timespan in ms after which a response is
     *                     expected at the latest
     */
    virtual void sendDeleteRequest(uint64_t destId, uint8_t cellOptions, int numCells,
                           std::vector<cellLocation_t> &cellList, int timeout) = 0;

    /**
     * @brief Send a 6P Relocation request.
     *
     * @param destId       Id of the node to which this request will be sent
     * @param cellOptions  The CellOptions as described in the 6P draft
     * @param numCells     The number of cells that are requested for allocation
     * @param relocationCellList    Cells that need to be relocated
     * @param candidateCellList     Cells that are proposed as alternatives (must
     *                              have length >= @param relocationCellList)
     * @param timeout      Timespan in ms after which a response is
     *                     expected at the latest
     */
    virtual void sendRelocationRequest(uint64_t destId, uint8_t cellOptions, int numCells,
                        std::vector<cellLocation_t> &relocationCellList,
                        std::vector<cellLocation_t> &candidateCellList,
                        int timeout) = 0;

    /**
     * @brief Send a 6P Clear request to @p destId.
     *
     * @param destId       Id of the node to which this request will be sent
     * @param timeout      Time at which this Request expires. Note that this is
     *                     NOT according to the standard (6P CLEARs don't have
     *                     timeouts, but otherwise the request might stay in limbo
     *                     forever)
     */
    virtual void sendClearRequest(uint64_t destId, int timeout) = 0;

    /**
     * @brief Piggyback @p payload onto any 6P message that is sent to @p destId.
     *        If no 6P message is scheduled to be sent to @p destId in the near
     *        future, send @p data in a dedicated SIGNAL message.
     *
     * @param destId       Id of the node to which data should be piggybacked
     * @param payload      Pointer to the data that should be piggybacked.
     *                     NOTE: The data itself will not be copied, so the data
     *                     behind the pointer should NOT be deleted or modified
     *                     after calling piggybackData()! When the piggybacked
     *                     data arrives at @p destId, the data behind the pointer
     *                     will be freed by the @p destId node.
     * @param payloadSz    The size the payload would have in the "real" packet
     *                     (in Bits)
     * @param timeout      Time (in ms) after which data MUST be sent as a Signal
     *                     request if it couldn't be piggybacked successfully.
     */
    virtual void piggybackData(uint64_t destId, void* payload, int payloadSz,
                               int timeout) = 0;

    /**
     * @brief set the @p option bit in @p cellOptions.
     */
    virtual void setCellOption(uint8_t* cellOptions, uint8_t option) = 0;
};

#endif /* __WAIC_TSCH6PINTERFACE_H */
