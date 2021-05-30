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
#include "RplDefs.h"
#include "inet/networklayer/common/InterfaceTable.h"


class TschMSF: public TschSF, public cListener {
    public:

    class SfControlInfo : public cObject {
        public:
            uint64_t reservedDestId;
            uint8_t cellOptions;

            SfControlInfo() {
                this->rtxCtn = 0;
            };
            SfControlInfo(uint64_t nodeId) {
                this->reservedDestId = nodeId;
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

        private:
            int numCells;
            int rtxCtn;
            tsch6pCmd_t cmd;
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
        uint8_t NumRx;

        friend std::ostream& operator<<(std::ostream& os, CellStatistic const& stat)
        {
            os << "TX: " << stat.NumTx
                    << ", ACKs: " << stat.NumTxAck
                    << ", RX: " << stat.NumRx << endl;
            return os;
        }
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
    void handleHousekeeping(cMessage* msg);
    void handleMaxCellsReached(cMessage* msg);

    /* unimplemented on purpose */
    void recordPDR(cMessage* msg) {}

    void receiveSignal(cComponent *src, simsignal_t id, cObject *value, cObject *details);
    void receiveSignal(cComponent *src, simsignal_t id, long value, cObject *details);

    void handleParentChangedSignal(uint64_t newParentId);
    void handlePacketEnqueued(uint64_t destId);

    /** DELAY_TEST: Add number of dedicated TX cells according to node's own rank
     * as well as the number of nodes in the network. Here a simple linear topology is assumed,
     * where number of node's descendants can be deduced based on the rank and # of hosts in the simulation
     *
     * @param rank Rpl rank of this node
     * @param numHosts number of hosts in the entire simulation
     * */
    void handleRplRankUpdate(long rank, int numHosts);

   protected:
    virtual void refreshDisplay() const override;

   private:
    const tsch6pSFID_t pSFID = SFID_MSF;

    /** the time after which an active, unfinished transaction expires
     *  (needs to be defined by every SF as per the 6P standard) */
    int pTimeout;

    /* the ID of this node, derived from its MAC address */
    uint64_t pNodeId;
    uint64_t rplParentId; // MAC of RPL preferred parent

    int numHosts; // DELAY_TEST: Number of hosts in the simulation

    bool hasOverlapping;
    int pHousekeepingPeriod;
    bool pHousekeepingDisabled;

    std::list<uint64_t> neighbors;

    cMessage *internalEvent;

    int pSlotframeLength;
    int pCellListRedundancy;
    int pNumChannels;
    offset_t pNumMinimalCells; // number of minimal cells being scheduled for ICMPv6, RPL broadcast messages

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
    cModule *rpl;
    cModule *hostNode; // reference to this host node's module

    /**
     * Set to True at index nodeId after @ref initNumCells cells have been
     * allocated successfully for link with nodeId
     */
    std::map<uint64_t, bool> initialScheduleComplete;


    /**
     * Stores status and retransmit counters of failed 6P transactions per neighbor
     */
    std::map<uint64_t, SfControlInfo*> pendingTransactions;

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
    std::vector<uint64_t> oneHopRplChildren;
    std::map<cellLocation_t, CellStatistic> cellStatistic;

    bool hasStarted;
    bool disable;

    /** Parameter variables, see NED */
    bool showTxCells;
    bool showQueueUtilization;
    bool showTxCellCount;

    cellLocation_t autoRxCell;

    simsignal_t rplParentChangedSignal;

    int rplRank;

    enum msfSelfMsg_t {
        CHECK_STATISTICS,
        REACHED_MAXNUMCELLS,
        DO_START,
        HOUSEKEEPING,
        DELAY_TEST,
        UNDEFINED
    };

    /**
     * @brief Create & send ADD request for @param numCells cells with option @param cellOptions
     */
    void addCells(uint64_t nodeId, int numCells, uint8_t cellOptions);
    void addCells(uint64_t nodeId, int numCells) { addCells(nodeId, numCells, MAC_LINKOPTIONS_TX); }

    void deleteCells(uint64_t nodeId, int numCells);
    void scheduleAutoCell(uint64_t neighbor);
    void scheduleAutoRxCell(InterfaceToken euiAddr);
    void removeAutoTxCell(uint64_t neighbor);
//    void relocateCell(cellLocation_t cell, double cellPdr, double maxPdr);
    void relocateCells(uint64_t neighbor);
    void relocateCells(uint64_t neighbor, cellLocation_t cell);
    void relocateCells(uint64_t neighbor, std::vector<cellLocation_t> relocCells);

    void clearScheduleWithNode(uint64_t sender);

    void scheduleMinimalCells();
    uint64_t checkInTransaction();
    bool checkOverlapping();

    /**
     * @return    true if @p slotOffset is already reserved,
     *            false otherwise
     */
    bool slotOffsetReserved(offset_t slOf);
    bool slotOffsetReserved(uint64_t nodeId, offset_t slOf);
    bool slOfScheduled(offset_t slOf);

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
    bool slotOffsetAvailable(offset_t slOf);

    /**
     * Pick @param numRequested items without duplicates randomly uniformly
     * from @param inputVec collection
     *
     * @return vector of picked slot offsets
     */
    std::vector<cellLocation_t> pickRandomly(std::vector<cellLocation_t> inputVec, int numRequested);

    /**
     * Check for free slot offsets (neither scheduled, nor reserved) in the range @param start -> @param end
     *
     * @return vector of free slot offsets
     */
    std::vector<offset_t> getAvailableSlotsInRange(offset_t start, offset_t end);
    std::vector<offset_t> getAvailableSlotsInRange(int slOffsetEnd);
};

#endif /*__WAIC_TSCHMSF_H_*/
