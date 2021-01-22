/*
 * Minimal Scheduling Function Implementation (6TiSCH WG Draft).
 *
 * Copyright (C) 2021  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2021  Yevhenii Shudrenko
 *           (C) 2019  Leo Krüger
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
#ifndef __WAIC_TSCHMSF_H_
#define __WAIC_TSCHMSF_H_

#include <omnetpp.h>

#include "Tsch6topSublayer.h"
#include "TschBlacklistManager.h"
#include "inet/networklayer/common/InterfaceTable.h"


class TschMSF: public TschSF, public cListener {

public:

    class MsfControlInfo : public cObject {
        public:
            uint64_t reservedDestId;

            MsfControlInfo() {};
            MsfControlInfo(uint64_t nodeId) {
                this->reservedDestId = nodeId;
            }

            uint64_t getNodeId() { return this->reservedDestId; }
            void setNodeId(uint64_t nodeId) { this->reservedDestId = nodeId; }

            int getNumDedicatedCells() { return this->numDedicatedCellsToAdd; }
            void setNumDedicatedCells(int numDedicated) { this->numDedicatedCellsToAdd = numDedicated; }

        private:
            int numDedicatedCellsToAdd;

    };

    virtual int numInitStages() const{return 6;}
    void initialize(int stage);
    void finish();
    void finish(cComponent *component, simsignal_t signalID) { cIListener::finish(component, signalID); }
    TschMSF();
    ~TschMSF();

    struct NbrStatistic {
        uint8_t NumCellsElapsed;
        uint8_t NumCellsUsed;
    };

    struct CellStatistic {
        uint8_t NumTx;
        uint8_t NumTxAck;
    };

    friend std::ostream& operator<<(std::ostream& os, std::vector<offset_t> const& slots)
    {
        for (auto s : slots)
            os << s << ", ";
        return os;
    }


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

    void handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells = -1,
                            std::vector<cellLocation_t> cellList = {});

    void handleSuccessResponse(uint64_t sender, tsch6pCmd_t lastKnownCmd, int numCells, std::vector<cellLocation_t> cellList);

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
    void handlePiggybackedData(uint64_t sender, void* data) {};

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
    void handleDoStart(cMessage* msg);
    void handleHouskeeping(cMessage* msg);
    void handleMaxCellsReached(cMessage* msg);

    /* unimplemented on purpose */
    void recordPDR(cMessage* msg) {}

    void receiveSignal(cComponent *src, simsignal_t id, cObject *value, cObject *details);
    void receiveSignal(cComponent *src, simsignal_t id, long value, cObject *details);
    void handleDodagJoinedSignal(uint64_t parentId);
    void handleParentChangedSignal(uint64_t newParentId);

private:
    const tsch6pSFID_t pSFID = SFID_MSF;

    /** the time after which an active, unfinished transaction expires
     *  (needs to be defined by every SF as per the 6P standard) */
    int pTimeout;

    /* the ID of this node, derived from its MAC address */
    uint64_t pNodeId;
    uint64_t rplParentId; // MAC of RPL preferred parent
    uint64_t rplFormerParentId; // and former parent (to track success/failure of clearing its schedule after leaving)


    std::list<uint64_t> neighbors;

    cMessage *internalEvent;

    int pSlotframeLen;
    int pNumChannels;
    offset_t pNumMinimalCells; // number of minimal cells being scheduled for ICMPv6, RPL broadcast messages


    int pMaxNumCells;
    int pMaxNumTx;
    double pLimNumCellsUsedHigh;
    double pLimNumCellsUsedLow;

    double routingParentTransactionDelay; // delay before trying to schedule dedicated TX with RPL preferred parent
    double pRelocatePdrThres;

    double clearReservedCellsTimeout;

    simtime_t SENSE_INTERVAL;

    /** Information about all links maintained by this node */
    TschLinkInfo* pTschLinkInfo;

    /** The 6P instance this SF is a part of.  */
    Tsch6topSublayer* pTsch6p;

    InterfaceTable* interfaceModule;

    Ieee802154eMac* mac;

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

    std::map<uint64_t, NbrStatistic> nbrStatistic;
    std::map<cellLocation_t, CellStatistic> cellStatistic;

    uint32_t autoRxSlOffset;

    bool hasStarted;
    bool unicastParentCellScheduled;
    bool disable;

    /**
     * Emitted each time the schedule setup with a neighbor is complete,
     * i.e. all cells have been allocated in both directions.
     */
    simsignal_t s_InitialScheduleComplete;

    cModule *hostNode; // reference to this host node's module

    enum msfSelfMessage_t {
        CHECK_STATISTICS,
        REACHED_MAXNUMCELLS,
        DO_START,
        HOUSEKEEPING,
        SCHEDULE_AUTO_TX,
        RESERVED_CELLS_TIMEOUT,
        UNDEFINED
    };

    /**
     * @brief Create & send ADD request to establish the first link to @nodeId
     */
    void addCells(uint64_t nodeId, int numCells);

    void deleteCells(uint64_t nodeId, int numCells);

    void scheduleAutoCell(uint64_t neighbor);
    void scheduleAutoRxCell(InterfaceToken euiAddr);
    void removeAutoTxCell(uint64_t neighbor);
//    void relocateCell(cellLocation_t cell, double cellPdr, double maxPdr);
    void relocateTxCells(cellVector cells);

    void clearScheduleWithNode(uint64_t sender);

    void retrySchedulingDedicatedCells(int numCells);

    void scheduleMinimalCells();
    uint64_t checkInTransaction();

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

    uint32_t saxHash(int maxReturnVal, InterfaceToken EUI64addr); // TODO: check this, often results in overlapping cells
    void clearCellStats(std::vector<cellLocation_t> cellList);
    void printCellUsage(std::string neighborMac, double usage);
    void updateCellTxStats(cellLocation_t cell, std::string statType);
    void updateNeighborStats(uint64_t neighbor, std::string statType);
    bool checkValidSlotRangeBounds(uint16_t start, uint16_t end);
    bool slotOffsetAvailable(offset_t slOf);

    void checkDedicatedCellScheduled(uint64_t sender, std::vector<cellLocation_t> cellList);

    void handleFailedTransaction(uint64_t sender, tsch6pCmd_t cmd);

    /** To track status of routing parent update and properly interpret RC_SUCCESS for CLEAR */
    bool parentUpdateInProgress;

    /**
     * Pick @param numRequested slot offsets randomly uniformly from @param availableSlots range
     * without duplicates
     *
     * @return vector of picked slot offsets
     */
    std::vector<offset_t> pickSlotOffsets(std::vector<offset_t> availableSlots, int numRequested);

    /**
     * Check for free slot offsets (neither scheduled, nor reserved) in the range @param start -> @param end
     *
     * @return vector of free slot offsets
     */
    std::vector<offset_t> getAvailableSlotsInRange(offset_t start, offset_t end);
    std::vector<offset_t> getAvailableSlotsInRange(int slOffsetEnd);
    std::vector<offset_t> getAvailableSlotsInRange() { return getAvailableSlotsInRange(pSlotframeLen); }
};

#endif /*__WAIC_TSCHCLSF_H_*/
