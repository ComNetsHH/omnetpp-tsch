/*
 * Minimal Scheduling Function Implementation (RFC9033).
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
#include "../TschHopping.h"
#include "inet/networklayer/common/InterfaceTable.h"


using namespace std;


class TschMSF: public TschSF, public cListener {
    public:

    class SfControlInfo : public cObject {

        private:
            int numCells;
            int rtxCtn;
            tsch6pCmd_t cmd;
            std::vector<cellLocation_t> cellList;

        public:
            uint64_t reservedDestId;
            uint8_t cellOptions;

            SfControlInfo() {
                this->rtxCtn = 0;
            };
            SfControlInfo(uint64_t nodeId) {
                this->reservedDestId = nodeId;
                this->numCells = 1;
                this->rtxCtn = 0;
            }

            uint64_t getNodeId() { return this->reservedDestId; }
            void setNodeId(uint64_t nodeId) { this->reservedDestId = nodeId; }

            uint64_t getCellOptions() { return this->cellOptions; }
            void setCellOptions(uint8_t cellOptions) { this->cellOptions = cellOptions; }

            int getNumCells() { return this->numCells; }
            void setNumCells(int numDedicated) { this->numCells = numDedicated; }

            tsch6pCmd_t get6pCmd() { return this->cmd; }
            void set6pCmd (tsch6pCmd_t cmd) { this->cmd = cmd; }

            std::vector<cellLocation_t> getCellList() { return this->cellList; };
            void setCellList(std::vector<cellLocation_t> cellList) { this->cellList = cellList; };

            int getRtxCtn() { return this->rtxCtn; }
            void setRtxCtn (int rtxCtn) { this->rtxCtn = rtxCtn; }

            int incRtxCtn() { this->rtxCtn++; return this->rtxCtn; }

            SfControlInfo* dup() {
                auto copy = new SfControlInfo();
                copy->set6pCmd(this->get6pCmd());
                copy->setCellOptions(this->getCellOptions());
                copy->setNumCells(this->getNumCells());
                copy->setNodeId(this->getNodeId());
                copy->setRtxCtn(this->getRtxCtn());
                return copy;
            }

            friend std::ostream& operator<<(std::ostream& os, SfControlInfo ci)
            {
                os << inet::MacAddress(ci.getNodeId()).str() << ": " << ci.get6pCmd() << " "
                        << ci.getNumCells() << " cells, retries - " << ci.getRtxCtn();
                return os;
            }
    };

    virtual int numInitStages() const override { return 6; }
    void initialize(int stage) override;
    void finish() override;
    void finish(cComponent *component, simsignal_t signalID) override { cIListener::finish(component, signalID); }
    TschMSF();
    ~TschMSF();

    class NbrStatistic: public cObject {

        public:
            uint8_t numCellsElapsed;
            uint8_t numCellsUsed;
            uint64_t macAddr;
//            cMessage* maxNumCellsMsg;

            NbrStatistic() {
                this->numCellsElapsed = 0;
                this->numCellsUsed = 0;
//                this->maxNumCellsMsg = new cMessage("MAX_NUM_CELLS", REACHED_MAXNUMCELLS);
//                maxNumCellsMsg->setContextPointer(new MacAddress(neighborId));
            }


            NbrStatistic(uint64_t nodeId) {
                this->numCellsElapsed = 0;
                this->numCellsUsed = 0;
                this->macAddr = nodeId;
            }


            friend std::ostream& operator<<(std::ostream& os, const NbrStatistic& stat)
            {
                os << "(" << inet::MacAddress(stat.macAddr) <<  ") elapsed = "
                        << (int) stat.numCellsElapsed << "; used = " << (int) stat.numCellsUsed;
                return os;
            }
    };

    struct CellStatistic {
        uint8_t NumTx;
        uint8_t NumTxAck;
        uint8_t NumRx;

        friend std::ostream& operator<<(std::ostream& os, CellStatistic const& stat)
        {
//            os << "TX: " << stat.NumTx
//                    << ", ACKs: " << stat.NumTxAck
//                    << ", RX: " << stat.NumRx << endl;
//            return os;

            if (stat.NumTx > 0)
                os << "PDR: " << (double) stat.NumTxAck / stat.NumTx * 100 << "%";
            else
                os << "No packets transmitted";

            return os;
        }
    };

    /**
     * @brief Start operating. This function MUST be called after creating a SF
     *        object, but not before all other nodes are initialized!
     *        (otherwise this SF object will be sending while other nodes are
     *        still unable to react correctly)
     */
    void start() override;

    /**
     * @return    the Scheduling Function Identifier (SFID) of this SF.
     */
    tsch6pSFID_t getSFID() override;

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
    int createCellList(uint64_t destId, std::vector<cellLocation_t> &cellList, int numCells) override;

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
                  int numCells, bool isRX, bool isTX, bool isSHARED) override;

    /**
     * @brief React to a 6P response.
     *
     * @param sender         The node that originated the response
     * @param code           The return code of the response to be handled
     * @param numCells       The value of the response's numCells field (if any).
     * @param cellList       The cellList contained in the response (if any)
     */
    void handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells = -1,
                        std::vector<cellLocation_t> *cellList = NULL) override;

    void handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells = -1,
                            std::vector<cellLocation_t> cellList = {}) override;

    virtual void handleTransactionTimeout(uint64_t sender) override;

    void handleDebugTestMsg();

    /**
     * This function gets called by 6top sublayer on receiving 6P CLEAR request,
     * the role of MSF is to erase all scheduled cells with
     */
    virtual void handle6pClearReq(uint64_t nodeId) override;

    virtual void freeReservedCellsWith(uint64_t nodeId) override;

    virtual void handleSuccessResponse(uint64_t sender, tsch6pCmd_t lastKnownCmd, int numCells, std::vector<cellLocation_t> cellList, vector<offset_t> reservedSlots);

    virtual void handleSuccessRelocate(uint64_t sender, std::vector<cellLocation_t> cellList);

    virtual void handleSuccessAdd(uint64_t sender, int numCells, vector<cellLocation_t> cellList, vector<offset_t> reservedSlots);

    void handleRplRankUpdate(long rank, int numHosts, double lambda);
    double getExpectedWaitingTime(int m) { return 1/((double) m + 1); }
    double getExpectedWaitingTime(int m, double pc, int rtx);
    int getRequiredServiceRate(double l) { return ceil(l + 0.001); }
    int getRequiredServiceRate(double l, double pc, int rtx, int rank, int numHosts);

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
    void handlePiggybackedData(uint64_t sender, void* data) override {} ;

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
    void handleInconsistency(uint64_t destId, uint8_t seqNum) override;

    /**
     * @return The 6P Timeout value defined by this Scheduling Function in ms
     */
    int getTimeout() override;

    offset_t getChOf(); // draw channel offset randomly uniformly sampled from the available range;
    void getHostModule();

    virtual void incrementNeighborCellElapsed(uint64_t neighborId) override;
    virtual void decrementNeighborCellElapsed(uint64_t neighborId) override;

    void handleMessage(cMessage* msg) override;
    void handleDoStart(cMessage* msg);
    void handleHousekeeping(cMessage* msg);
    void virtual handleMaxCellsReached(cMessage* msg);

    /* unimplemented on purpose */
    void recordPDR(cMessage* msg) override {}

    void receiveSignal(cComponent *src, simsignal_t id, cObject *value, cObject *details) override;
    void receiveSignal(cComponent *src, simsignal_t id, long value, cObject *details) override;
    void receiveSignal(cComponent *src, simsignal_t id, const char *s, cObject *details) override;

    /**
     * Process RPL preferred parent updates
     *
     * Whenever a new preferred parent is selected, we have to schedule dedicated TX cell to it
     *
     * @param newParentId MAC address of the new parent
     */
    void handleParentChangedSignal(uint64_t newParentId);

    /**
     * Process packet enqueue event coming from the MAC layer.
     * Ensures there's a TX cell to this destination
     * by adding an auto cell if there's no dedicated / auto cells present.
     *
     * @param destId MAC address of the destination for which there's a packet enqueued
     */
    virtual void handlePacketEnqueued(uint64_t destId);

    // get list of cells with preferred parent to delete synchronously
    std::vector<cellLocation_t> getCellsToDeleteSync(uint64_t nodeId, int numCellsReq);

    // delete cells from the schedule and tsch link info directly without transactions
    void deleteCellsSync(uint64_t nodeId, std::vector<cellLocation_t> cellList);
    void handleBurstFinishedProcessingSignal();

   protected:
    virtual void refreshDisplay() const override;

    const tsch6pSFID_t pSFID = SFID_MSF;

    /** the time after which an active, unfinished transaction expires
     *  (needs to be defined by every SF as per the 6P standard) */
    int pTimeout;

    /* the ID of this node, derived from its MAC address */
    uint64_t pNodeId;
    uint64_t rplParentId; // MAC of RPL preferred parent

    bool hasOverlapping;
    int pHousekeepingPeriod;
    bool pHousekeepingDisabled;

    std::list<uint64_t> neighbors;

    cMessage *internalEvent;

    int pSlotframeLength;
    int pCellListRedundancy;
    int pNumChannels;
    int pCellIncrement; // see .ned parameter "cellBandwidthIncrement"
    bool pSend6pDelayed; // see .ned parameter
    offset_t pNumMinimalCells; // number of minimal cells being scheduled for ICMPv6, RPL broadcast messages
    bool pBlacklistingEnabled;
    int pChOfStart;
    int pChOfEnd;

    bool isSink;

    int numCellsRequired; // for delay testing scenarios only
    int pMaxNumCells;
    int pMaxNumTx;
    int tsch6pRtxThresh;
    double pLimNumCellsUsedHigh;
    double pLimNumCellsUsedLow;
    double pRelocatePdrThres;

    simtime_t SENSE_INTERVAL;

    /** Information about all links maintained by this node */
    TschLinkInfo* pTschLinkInfo;

    /** The 6P instance this SF is a part of.  */
    Tsch6topSublayer* pTsch6p;

    InterfaceTable* interfaceModule;

    Ieee802154eMac *mac;
    TschSlotframe *schedule;
    cModule *rpl; // TODO: use actual RPL pointer
    cModule *hostNode;
    TschHopping* hopping;


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

    std::map<uint64_t, NbrStatistic*> nbrStatistic;
    std::map<uint64_t, cMessage*> maxNumCellsMessages;

    std::vector<uint64_t> oneHopRplChildren;
    std::map<cellLocation_t, CellStatistic> cellStatistic;
    std::map<uint64_t, std::vector<offset_t>> blacklistedSlots;
    std::map<uint64_t, SfControlInfo*> retryInfo; // stores info about outgoing 6P requests to enable retries
    // stores info about nodes for whom downlink has been requested
    // to avoid DAO retransmissions spawning more cells than necessary
    std::map<uint64_t, int> downlinkRequested;

    // Stats
    int numInconsistencies;
    int numLinkResets;
    int numFailedTracked6p;
    int num6pAddSent;
    int num6pAddFailed;
    int numUnhandledResponses;
    int numTranAbandonedMaxRetries;
    int numTranAbortedUnknownReason;
    double util; // queue utilization with preferred parent
    double uplinkCellUtil; // cell utilization with pref. parent

    cMessage *delayed6pReq;

    bool hasStarted;
    bool isDisabled;
    bool isLeafNode; // based on RPL info


    /** Parameter variables, see NED */
    bool showTxCells;
    bool showQueueUtilization;
    bool showQueueSize;
    bool showTxCellCount;
    bool showLinkResets;
    bool pCellBundlingEnabled;
    int pCellBundleSize;
    int udpPacketsSent;

    cellLocation_t autoRxCell;
    cellLocation_t initCellOverride; // override random init cell scheduled with preferred RPL parent

    int rplRank;


    // Low-latency scheduling (CLXv2)
    offset_t uplinkSlotOffset; // slot offset of the preferred parent to schedule close to
    simsignal_t uplinkScheduledSignal; // used to notify RPL about the slot offset of the scheduled uplink cell

    // cell matching statistics
    simsignal_t uncoverableGapSignal;

    // bursty traffic modeling
    simsignal_t deleteCellsSyncSignal;

    enum msfSelfMsg_t {
        CHECK_STATISTICS,
        REACHED_MAXNUMCELLS,
        QUEUE_ESTIMATION,
        DO_START,
        HOUSEKEEPING,
        SEND_6P_REQ,
        DELAY_TEST,
        CHECK_DAISY_CHAIN, // message type for CLX scheduling
        DEBUG_TEST,
        SCHEDULE_UPLINK,
        SCHEDULE_DOWNLINK,
        CELL_BUNDLE_REQ,
        CHANGE_SLOF,
        DISABLE_ADAPTATION,
        UNDEFINED
    };

    /**
     * @brief Create & send ADD request for @param numCells cells with option @param cellOptions
     *
     * @param delay (optional) additional timeout before sending out the request message
     */
    bool addCells(uint64_t nodeId, int numCells, uint8_t cellOptions);
    bool addCells(SfControlInfo *retryInfo);

    virtual void deleteCells(uint64_t nodeId, int numCells);
    void scheduleAutoCell(uint64_t neighbor);
    void scheduleAutoRxCell(InterfaceToken euiAddr);

    void removeAutoTxCell(uint64_t neighbor);

    virtual void handleSelfMessage(cMessage* msg);
    void handleCellBundleReq();
    void handleDeleteCellsSync(std::vector<int> slofsToDelete, uint64_t nbrId = 0, uint8_t linkOption = MAC_LINKOPTIONS_RX);

    // TODO: revise whether it makes sense to have both of these
    void handleScheduleUplink();
    void handleScheduleDownlink(uint64_t nodeId);
    void retryLastTransaction(uint64_t nodeId, std::string reasonStr);

    /**
     * Sends out 6P request according to the details of SfControlInfo object.
     * This function serves as a handler for self-msg event for sending 6P messages with random delay.
     *
     * @param ctrlInfo control info object containig all the details about the 6P request to be sent
     */
    void send6topRequest(SfControlInfo *ctrlInfo);

    /**
     * Relocate single cell scheduled with the neighbor at specified location
     *
     * @param neighbor MAC address of the neighbor to relocate cells with
     * @param cell location of cell to be relocated
     */
    void relocateCells(uint64_t neighbor, cellLocation_t cell);

    /**
     * Relocate multiple cells scheduled with the neighbor
     *
     * @param neighbor MAC address of the neighbor to relocate cells with
     */
    virtual void relocateCells(uint64_t neighbor, std::vector<cellLocation_t> relocCells);

    /**
     * Remove cells from the schedule as well as corresponding cell statsistics associated with neighbour
     *
     * @param sender MAC address of the neighbour to clear schedule with
     */
    void clearScheduleWithNode(uint64_t sender); // TODO: create a dedicated function directly in TschSlotframe

    /**
     * Schedule minimal cells (TX RX SHARED) for broadcast and control messages
     *
     * @param numMinimalCells number of cells to schedule evenly across the slotframe length
     * @param slotframeLength length of the slotframe in timeslots
     */
    void scheduleMinimalCells(int numMinimalCells, int slotframeLength);

    uint64_t checkInTransaction();
    bool checkOverlapping();

    /**
     * @return    true if @p slotOffset is already reserved,
     *            false otherwise
     */
    bool slotOffsetReserved(offset_t slOf);
    bool slotOffsetReserved(uint64_t nodeId, offset_t slOf);
    bool slOfScheduled(offset_t slOf);
    bool isLossyLink(); // TEST, only for manual lossy link testing
    bool pCheckScheduleConsistency;

    /**
     * @return    the number of times the channel has been busy in %.
     */
    //double calcBusyness(std::vector<ChannelState> states);

    uint32_t saxHash(int maxReturnVal, InterfaceToken EUI64addr); // TODO: check this, often results in overlapping cells
    void clearCellStats(std::vector<cellLocation_t> cellList);
    std::string printCellUsage(std::string neighborMac, double usage);
    void updateCellTxStats(cellLocation_t cell, std::string statType);

    void removeCell(uint64_t neighbor, cellLocation_t cell, uint8_t cellOptions);

    void updateNeighborStats(uint64_t neighbor, std::string statType);
    void checkMaxCellsReachedFor(uint64_t neighborId);

    bool slotOffsetAvailable(offset_t slOf);

    simsignal_t queueUtilization;
    simsignal_t failed6pAdd; // tracks number of failed 6P ADD requests
    simsignal_t neighborNotFoundError; // tracks unknown error where node's schedule is not cleared properly
    simsignal_t uplinkSlotOffsetSignal; // emits the slot offset after a dedicated cell is scheduled with preferred parent

    enum transactionFailReason_t {
        TIMEOUT,
        EMPTY_CELLLIST
    };

    class TransactionFailDetails : public cObject {
        private:
            transactionFailReason_t failReason;
            MacAddress nbr;

        public:
            TransactionFailDetails() {}
            TransactionFailDetails(transactionFailReason_t failReason, MacAddress nbr) {
                this->failReason = failReason;
                this->nbr = nbr;
            }
    };



    /**
     * Pick @param numRequested items without duplicates randomly uniformly
     * from @param inputVec collection
     *
     * @return vector of picked slot offsets
     */
    std::vector<cellLocation_t> pickRandomly(std::vector<cellLocation_t> inputVec, int numRequested);
    std::vector<cellLocation_t> pickConsecutively(std::vector<cellLocation_t> inputVec, int numRequested, bool randomizeStart);
    std::vector<cellLocation_t> pickSpaceBetween(std::vector<cellLocation_t> inputVec, int numRequested);

    /**
     * Check for free slot offsets (neither scheduled, nor reserved) in the range @param start -> @param end
     *
     * @return vector of free slot offsets
     */
    std::vector<offset_t> getAvailableSlotsInRange(int start, int end);
    std::vector<offset_t> getAvailableSlotsInRange(int slOffsetEnd);

    int pInitNumRx;

    /**
     * Spaghetti function to check that information about a neighbor stored in TschSlotframe
     * and TschLinkInfo is the same
     *
     * @param nodeId MAC identifier of the neighbor node
     */
    void checkScheduleConsistency(uint64_t nodeId);
    double getCoverageRate();
};

#endif /*__WAIC_TSCHMSF_H_*/
