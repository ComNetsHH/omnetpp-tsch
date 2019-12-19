/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 * Stores and handles information about all links maintained by
 * this node.
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

#ifndef __WAIC_TSCHLINKINFO_H_
#define __WAIC_TSCHLINKINFO_H_

#include <omnetpp.h>
#include <algorithm>
#include "WaicCellComponents.h"
#include "Tsch6tischComponents.h"
#include "tschLinkInfoTimeoutMsg_m.h"

using namespace omnetpp;

class TschLinkInfo: public cSimpleModule
{
    /**
     * Information about the link to another node.
     */
    typedef struct {
        uint64_t nodeId;                   /**< Address of the Node we're sharing a link with */
        bool inTransaction;           /**< Whether there's currently an
                                           unfinished transaction going on */
        tschLinkInfoTimeoutMsg *tom;  /**< Msg scheduled for when the timeout
                                           of the currently active transaction
                                           (if any) expires */
        uint8_t lastKnownSeqNum;      /**< The current link seqNum, as seen by this node */
        tsch6pMsg_t lastKnownType;    /**< When in transaction: packet type last
                                           sent/received */
        tsch6pCmd_t lastKnownCommand; /**< When in transaction: command type last
                                           sent/received */
        uint8_t lastLinkOption;/**< For transactions initiated by this node:
                                           The value of the linkOption field in the
                                           Request we sent */
        cellVector scheduledCells;    /**< The cells scheduled between this node and nodeId */
        cellVector relocationCells;   /**< Cells that this node requested relocate.
                                           Is only non-empty during a RELOCATE
                                           transaction started by this node. */
        //std::mutex nliMutex;          /**< Prevents SF & 6P from read/editing this entry
        //                                   at the same time */
    } NodeLinkInfo_t;

public:
    /* TODO: make this a singleton?! */
    TschLinkInfo();
    ~TschLinkInfo();
    void initialize(int stage);

    /**
     * @return    true if a linkInfo entry exists for @p nodeId,
     *            false otherwise
     */
    bool linkInfoExists(uint64_t nodeId);

    /**
     * @brief Get all neighbors this node shares cells with.
     *
     * @return    The nodeId s of all neighbors to whom a link exists
     */
    std::vector<uint64_t> getLinks();

    /**
     * @brief TODO
     * @return    0 on success,
     *            -EEXIST if there already is a link to @p nodeId
     */
    int addLink(uint64_t nodeId, bool inTransaction, simtime_t transactionTimeout,
                uint8_t lastKnownSeqNum);

    /**
     * @brief Abort active transaction with @p nodeId (if any),
     *        clear all cells, reset the sequence number and set lastLnownType
     *        to @p lastKnownType.
     *        If no link to @p nodeId exists, this method does nothing.
     */
    void resetLink(uint64_t nodeId, tsch6pMsg_t lastKnownType);

    /**
     * @brief Abort active transaction with @p nodeId (if any),
     *        set sequence number to previous value and set lastLnownType
     *        to @p lastKnownType.
     *        If no link to @p nodeId exists, this method does nothing.
     */
    void revertLink(uint64_t nodeId, tsch6pMsg_t lastKnownType);

    /**
     * @return    true if @p nodeId is currently involved in an unfinished transaction
     *            false otherwise
     */
    bool inTransaction(uint64_t nodeId);

    /**
     * @brief Mark link with @p nodeId as "in transaction", i.e. an unfinished
     *        transaction exists.
     *
     * @param nodeId              TODO
     * @param transactionTimeout  Timeout in absolute time
     * @return    0 on success,
     *            -EINVAL if no link to nodeId exists
     */
    int setInTransaction(uint64_t nodeId, simtime_t transactionTimeout);

    /**
     * @brief Mark link with @p nodeId as "not in transaction"
     *
     * @param nodeId              TODO
     * @return    0 on success,
     *            -EINVAL if no link to nodeId exists
     */
    int abortTransaction(uint64_t nodeId);

    /**
     * @brief Add cell for link with @p nodeId.
     * @return    0 on success,
     *            -EINVAL otherwise
     */
    int addCell(uint64_t nodeId, cellLocation_t cell, uint8_t linkOption);

    /**
     * @brief Add cells for link with @p nodeId.
     *
     * @param linkOption   LinkOption that applies to *all* added cells
     * @return    0 on success,
     *            -EINVAL otherwise
     */
    int addCells(uint64_t nodeId, const std::vector<cellLocation_t> &cellList,
                 uint8_t linkOption);

    /**
     * @return all cells scheduled for the link with @p nodeId.
     */
    cellVector getCells(uint64_t nodeId);

    /**
     * @return the associated cell options or 0xFF
     */
    uint8_t getCellOptions(uint64_t nodeId, cellLocation_t candidate);

    /**
     * @return the number of cells scheduled between this ode and @p nodeId
     */
    int getNumCells(uint64_t nodeId);

    /**
     * @return the neighbor belonging to the cell or 0
     */
    uint64_t getNodeOfCell(cellLocation_t candidate);

    /**
     * @brief Delete all cells that are scheduled on the link with @p nodeId.
     */
    void clearCells(uint64_t nodeId);

    /**
     * @brief Delete the cells specified in @p cellList from the link with
     *        @p nodeId, provided they match @p linkOption.
     */
    void deleteCells(uint64_t nodeId, const std::vector<cellLocation_t> &cellList,
                     uint8_t linkOption);

    /**
     * @return    true if any cell is scheduled at @p timeOffset
     *            false otherwise
     */
    bool timeOffsetScheduled(offset_t timeOffset);

    /**
     * @brief Check if all cells in @p cellList are scheduled on the link with
     *        @p nodeId, and with a matching @p linkOption.
     *
     * @return    true if all cells and linkOptions match,
    *             false otherwise
     */
    bool cellsInSchedule(uint64_t nodeId, std::vector<cellLocation_t> &cellList,
                         uint8_t linkOption);

    /**
     * @brief Store the cells that this node is going to ask @p nodeId
     *        to relocate.
     *
     * @return    0 on success,
     *            -EINVAL if there's no link to destId or there's an open transaction.
     */
    int setRelocationCells(uint64_t nodeId, std::vector<cellLocation_t> &cellList,
                           uint8_t linkOption);

    /**
     * @brief Get the cells that this node asked  @p nodeId to relocate.
     *
     * @return    All cells that were suggested to @p nodeId, if any.
     *            (returns an empty vector if no relocation was initiated previously)
     */
    std::vector<cellLocation_t> getRelocationCells(uint64_t nodeId);

    /**
     * @brief Replace the cells from this link's relocationCells register (set
     *        by an earlier call to @ref setRelocationCells()) with the cells
     *        from @p newCells (all with linkOption @p linkOption).
     *        If the relocationCells register is empty, this method changes nothing.
     *
     * @return    0 on success,
     *            -EINVAl if no link to @ nodeId exists or we didn't start a
     *            RELOCATE transaction
     */
    int relocateCells(uint64_t nodeId, std::vector<cellLocation_t> &newCells,
                      uint8_t linkOption);

    /**
     * @return    the last known sequence number of @p nodeId if a link exists,
     *            0 otherwise
     */
    uint8_t getLastKnownSeqNum(uint64_t nodeId);

    /**
     * @return     The current sequence number for the link to @p nodeId
     */
    uint8_t getSeqNum(uint64_t nodeId);

    /**
     * @brief Increment the Sequence Number of the link to @p nodeId by 1.
     *        Note that the Sequence Number is a lollipop counter: If seqNum
     *        is 0xFF, seqNum + 1 is 0x01 (not 0x00).
     */
    void incrementSeqNum(uint64_t nodeId);

    /**
     * @brief Reset the Sequence Number of the link to @p nodeId
     *       (i.e. set it to 0.)
     */
    void resetSeqNum(uint64_t nodeId);

    /**
     * @param type The type of 6P message last received on the link to @p nodeId
     * @return    0 on success,
     *            -EINVAL otherwise
     */
    int setLastKnownType(uint64_t nodeId, tsch6pMsg_t type);

    /**
     * @return    The type of 6P message last received on the link to @p nodeId
     */
    tsch6pMsg_t getLastKnownType(uint64_t nodeId);

    /**
     * @param cmd          The type of 6P command last received or sent on the
     *                     link to @p nodeId
     *
     * @return             0 on success,
     *                     -EINVAL otherwise
     */
    int setLastKnownCommand(uint64_t nodeId, tsch6pCmd_t cmd);

    /**
     * @return    The type of 6P command last received on the link to @p nodeId
     */
    tsch6pCmd_t getLastKnownCommand(uint64_t nodeId);

    /*
     * @param linkOption   The linkOption set in the last request sent by this
     *                     node to @p nodeId.
     *
     * @return             0 on success,
     *                     -EINVAL otherwise
     */
    int setLastLinkOption(uint64_t nodeId, uint8_t linkOption);

    /*
     * @return             The linkOption set in the last request sent by this
     *                     node to @p nodeId if a link to @p nodeId exists,
     *                     MAC_LINKOPTIONS_NONE otherwise.
     */
    uint8_t getLastLinkOption(uint64_t nodeId);

private:

    /** Gate to send timeout notifications to 6P */
    GateId sublayerControlOut;

    /**
     * Information about all links maintained by this node, indexed by nodeId.
     * Contains one entry per neighbor.
     */
    std::map<uint64_t, NodeLinkInfo_t> linkInfo;

    /**
     * @brief Start the timeout "countdown" for the current transaction with
     *        @p nodeId.
     *
     * @param timeout      The simtime at which the transaction times out
     */
    void startTimeoutTimer(uint64_t nodeId, simtime_t timeout);

    /**
     * @brief Used to handle self-messages, i.e. delayed timeout messages sent by
     *        startTimeoutTimer().
     */
    void handleMessage(cMessage *msg);

    bool matchingTimeOffset(std::tuple<cellLocation_t, uint8_t> const& obj,
                                      offset_t timeOffset);
};

#endif /*__WAIC_TSCHLINKINFO_H_*/
