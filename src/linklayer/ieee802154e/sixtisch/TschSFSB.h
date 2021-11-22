/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 * Implementation of our Scheduling Function with Soft Blacklisting
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

#ifndef __WAIC_TSCHSFSB_H_
#define __WAIC_TSCHSFSB_H_

#include <omnetpp.h>

#include "Tsch6topSublayer.h"
#include "TschLinkInfo.h"
#include "TschSF.h"
#include "TschBlacklistManager.h"
#include "TschSpectrumSensing.h"


class TschSFSB: public TschSF {
    /** @brief Holds the blacklist update data that is piggybacked onto 6P messages */
    struct blacklistUpdate_t {
        uint8_t header;      /**< Specifies whether the entries have start/end/period fields */
        std::vector<TschBlacklistManager::blacklistEntry_t> entries; /**< New
                                                    entries for the blacklist,
                                                    one per channel */
    };

public:
    virtual int numInitStages() const{return 6;}
    void initialize(int stage);
    TschSFSB();
    ~TschSFSB();

    /**
     * @brief Start operating. This function MUST be called after creating a SF
     *        object, but not before all other nodes are initialized!
     *        (otherwise this SF object will be sending while other nodes are
     *        still unable to react correctly)
     */
    void start();

    /**
     * @return    the Scheduling Function Identifier (SFID) of this SF.
     */
    tsch6pSFID_t getSFID();

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
    int createCellList(uint64_t destId, std::vector<cellLocation_t> &cellList, int numCells);

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
    int pickCells(uint64_t destId, std::vector<cellLocation_t> &cellList,
                  int numCells, bool isRX, bool isTX, bool isSHARED);

    /**
     * @brief React to a 6P response.
     *
     * @param sender         The node that originated the response
     * @param code           The return code of the response to be handled
     * @param numCells       The value of the response's numCells field (if any).
     * @param cellList       The cellList contained in the response (if any)
     */
    void handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells = -1,
                        std::vector<cellLocation_t> *cellList = NULL);

    /**
     * @brief More "nullptr exceptions"-safe version of handleResponse().
     *
     * @overload
     */
    void handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells = -1,
                                std::vector<cellLocation_t> cellList = {}) {}

    void handleTransactionTimeout(uint64_t sender) {}

    /**
     * @brief Handle @p data that was piggybacked by @p sender.
     *
     * @param sender         The node that sent the data
     * @param data           Pointer to the data that was piggybacked. Note that
     *                       the message only contained the pointer, NOT the data
     *                       (this works because this is not the real world ;)).
     *                       It is the task of the SF to explicitly free the data
     *                       behind the pointer once it's done processing it.
     */
    void handlePiggybackedData(uint64_t sender, void* data);

    /**
     * @brief Handle an update from the @ref TschSpectrumSensing module.
     *        These updates will arrive after each completed spectrum sweep.
     *        Might trigger a cell relocation.
     *
     * @param ssr            The results of one sweep, i.e. fresh channel
     *                       quality information for each channel in use
     */
    //void handleSpectrumSensingResult(tschSpectrumSensingResult* ssr);

    /**
     * @brief Handle the inconsistency which was uncovered by @p seqnum
     *        in the schedule maintained with @p destId
     */
    void handleInconsistency(uint64_t destId, uint8_t seqNum);

    /**
     * @return The 6P Timeout value defined by this Scheduling Function in ms
     */
    int getTimeout();

    void handleMessage(cMessage* msg);

    /* unimplemented on purpose */
    void recordPDR(cMessage* msg) {}

private:
    // TODO: refactor into enum, this is a mess
    /**
     * The space (in Bits) the header field of an entire blacklist update would
     * take up in a "real" packet (needed to update the bitLength of piggybacked
     * 6P messages)
     */
    const int blacklistupdateHdrSz = 3;

    /**
     * The space (in Bits) the channel field of one blacklist update entry would
     * take up in a "real" packet (needed to update the bitLength of piggybacked
     * 6P messages)
     */
    const int blacklistupdateChannelSz = 4;

    /**
     * The space (in Bits) a start/end/period field of one blacklist update entry
     * would take up in a "real" packet (needed to update the bitLength of
     * piggybacked 6P messages)
     * currently unused because we don't explicitly
     * send timestamps-- we only use the default ones.
     */
    const int blacklistupdateTimestampSz = 8;

    /** The Identifier of this Scheduling Function*/
    const tsch6pSFID_t pSFID = SFID_SFSB;

    /** the time after which an active, unfinished transaction expires
     *  (needs to be defined by every SF as per the 6P standard) */
    int pTimeout;

    /** The number of Spectrum Sensing results to consult for a blacklisting decision */
    int numSensingResults;

    /** If the channel has been busy >= blacklistThreshold % of the last
        @p numSensingResults, it should be blacklisted. */
    double blacklistThreshold;

    /** The number of cells (ideally) initially shared with each neighbor.  */
    int INIT_NUM_CELLS;

    /** Time (in ms) after which data MUST be sent as a Signal request if it couldn't be
     *  piggybacked successfully. */
    int BLACKLIST_UPDATE_TIMEOUT;

    /** The default lifetime (in ms) of a blacklist entry, if nothing else is
     *  specified in the blacklist update */
    simtime_t BLACKLIST_LIFETIME;

    /** The maximum time in msthat will pass until another initial ADD request
     *  is sent if the last one wasn't successful */
    int MAX_RETRY_BACKOFF;

    /* the ID of this node, derived from its MAC address */
    uint64_t pNodeId;

    int pSlotframeSize;
    int pNumChannels;

    /** If set to true, SFSB will do nothing other than the initial cell negotiation
     *  (i.e. performs blind channel hopping with dedicated cells) */
    bool blindHopping;

    simtime_t SENSE_INTERVAL;
    /**
     * Disable the functionality of TschSFSB
     */
    bool disable;

    //TODO: Needs to be updated
    /** The last @p numSensingResults per channel, indexed by channel number */
    //std::map<int, std::vector<ChannelState>> channelHistory;

    /** The blacklist updates that were sent by neighbors but can't yet be used
        because no link has been established, indexed by neighbor id */
    std::map<int,std::vector<TschBlacklistManager::blacklistEntry_t>> pendingBlacklistUpdates;

    /** Information about all links maintained by this node */
    TschLinkInfo* pTschLinkInfo;

    /** The 6P instance this SF is a part of.  */
    Tsch6topSublayer* pTsch6p;

    /** The MAC layer module that manages channel blacklisting */
    TschBlacklistManager* pTschBlacklistManager;

    /**
     * Set to True at index nodeId after @ref initNumCells cells have been
     * allocated successfully for link with nodeId
     */
    std::map<uint64_t, bool> initialScheduleComplete;

    /**
     * TimeOffsets that have been suggested to a neighbor in an unfinished ADD
     * or RELOCATE transactions (and can thus currently not be suggested to any
     * other neighbor).
     *
     * Each key is the nodeId the timeOffsets were suggested to. The value
     * stores the last suggested set of timeOffsets. Because only one Transaction
     * can be active at a time, <= 1 set of timeOffsets will be stored per node.
     */
    std::map<uint64_t, std::vector<offset_t>> reservedTimeOffsets;

    /**
     * Emitted each time the schedule setup with a neighbor is complete,
     * i.e. all cells have been allocated in both directions.
     */
    simsignal_t s_InitialScheduleComplete;

    enum sfsbSelfMessage_t {
        SEND_INITIAL_ADD,
        DO_SENSE,
    };

    /**
     * @brief Create & send ADD request to establish the first link to @nodeId
     */
    void sendInitialAdd(uint64_t nodeId);

    /**
     * @return    true if @p timeOffset is already reserved,
     *            false otherwise
     */
    bool timeOffsetReserved(offset_t timeOffset);

    /**
     * @return    true if @p timeOffset was reserved by @p nodeId,
     *            false otherwise
     */
    bool timeOffsetReservedByNode(uint64_t nodeId, offset_t timeOffset);

    /**
     * @return    the number of times the channel has been busy in %.
     */
    //double calcBusyness(std::vector<ChannelState> states);
};

#endif /*__WAIC_TSCHSFSB_H_*/
