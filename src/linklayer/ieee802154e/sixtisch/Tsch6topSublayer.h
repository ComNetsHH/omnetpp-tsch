/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 * Implementation of the 6top sublayer as a MiXiM BaseApplLayer.
 *
 * Note that the 6top sublayer isn't *actually* located on layer 7, but this is
 * the only interface that lets you not have a gate to any upper layer. And since
 * the 6top sublayer serves as kind of a mini application layer to the MAC layer,
 * (at least for now, in case it communicates with the routing protocol for
 * optimization purposes this probably has to change) this is what it is now.
 *
 * For more details on the sublayer see
 * https://tools.ietf.org/html/draft-ietf-6tisch-architecture
 *
 * Copyright (C) 2021  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2021  Yevhenii Shudrenko
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

#ifndef __WAIC_TSCHNETLAYER_H_
#define __WAIC_TSCHNETLAYER_H_

#include <omnetpp.h>

//#include "BaseApplLayer.h"
#include "inet/applications/base/ApplicationBase.h"

#include "tsch6pPiggybackTimeoutMsg_m.h"
#include "SixpHeaderChunk_m.h"
#include "SixpDataChunk_m.h"
#include "tsch6topCtrlMsg_m.h"

#include "Tsch6pInterface.h"
#include "Tsch6tischComponents.h"

#include "TschSF.h"
#include "TschLinkInfo.h"
#include "../Ieee802154eMac.h"
#include "../TschSlotframe.h"

using namespace tsch;
using namespace inet;

class Tsch6topSublayer: public ApplicationBase, public Tsch6PInterface, public cListener {
public:
    Tsch6topSublayer();
    ~Tsch6topSublayer();

    virtual int numInitStages() const override {return 5;}

    virtual void initialize(int) override;
    virtual void finish() override;

    /**
     * @brief Handle messages containing 6top data that are sent over from
     *        the MAC layer.
     *        If @p msg is a Tsch6pPacket, handle it according to
     *        the 6P specifications. If it is a tschLinkInfoTimeoutMsg, handle
     *        the respective timeout.
     *
     * @param msg          The message to be handled
     */
    virtual void handleMessage(cMessage* msg) override;

    virtual Packet* handleSelfMessage(cMessage* msg);
    virtual Packet* handleExternalMessage(cMessage* msg);

    /**
     * Check the status of piggybackable data on link-layer (LL) ACK reception
     */
    virtual void handlePiggybackData(uint64_t destId, bool txSuccess);

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
    bool sendAddRequest(uint64_t destId, uint8_t cellOptions, int numCells,
                        std::vector<cellLocation_t> &cellList, int timeout) override;
    bool sendAddRequest(uint64_t destId, uint8_t cellOptions, int numCells,
                            std::vector<cellLocation_t> &cellList, int timeout, double delay);

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
    void sendDeleteRequest(uint64_t destId, uint8_t cellOptions, int numCells,
                           std::vector<cellLocation_t> cellList, int timeout) override;

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
    void sendRelocationRequest(uint64_t destId, uint8_t cellOptions, int numCells,
                        std::vector<cellLocation_t> &relocationCellList,
                        std::vector<cellLocation_t> &candidateCellList,
                        int timeout) override;

    /**
     * @brief Send a 6P Clear request to @p destId.
     *
     * @param destId       Id of the node to which this request will be sent
     * @param timeout      Time at which this Request expires. Note that this is
     *                     NOT according to the standard (6P CLEARs don't have
     *                     timeouts, but otherwise the request might stay in limbo
     *                     forever)
     */
    void sendClearRequest(uint64_t destId, int timeout) override;

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
    void piggybackData(uint64_t destId, void* payload, int payloadSz, int timeout) override;

    /**
     * @brief set the @p option bit in @p cellOptions.
     */
    void setCellOption(uint8_t* cellOptions, uint8_t option) override;

    /**
     * @brief Get list of all neighbors in transmission range
     *
     * @param pnodeId ID of the node
     * @param mac pointer to the maclayer
     */
    std::list<uint64_t> getNeighborsInRange(uint64_t pnodeId, Ieee802154eMac* mac);

    /**
     * @brief Update the schedule of the MAC layer
     */
    void updateSchedule(tsch6topCtrlMsg msg);

    /** TSCH schedule */
    TschSlotframe *schedule;
    /** MAC */
    Ieee802154eMac* mac;

    // RPL router
    cModule *host;


protected:
    /**
     * @brief handles a lower ctrl message.
     */
    virtual void handleLowerControl(cMessage * msg);

    virtual void handleMessageWhenUp(cMessage *msg) override{}; //TODO Not needed, could be changed if needed
    virtual void handleStartOperation(LifecycleOperation *operation) override{}; //TODO Not needed, could be changed if needed
    virtual void handleStopOperation(LifecycleOperation *operation) override{}; //TODO Not needed, could be changed if needed
    virtual void handleCrashOperation(LifecycleOperation *operation) override{}; //TODO Not needed, could be changed if needed

private:
    /**
     * The space (in Bits) that 1 entry in a cellList takes up in a 6P message header
     */
    const int cellHdrSz = 32;

    /**
     * Size of the base header contained in all 6P messages (containing
     * 6p Version, Type, Reserved bits, Code, SFID, and SeqNum) in *Bits*
     */
    const int baseMsgHdrSz = 32;

    /**
     * Size of the fixed parts of the additional ADD or RELOCATE Request header
     * (i.e. Metadata, CellOptions, NumCells) in *Bits*
     */
    const int addDelRelocReqMsgHdrSz = 32;

    /**
     * The minimum amount of time to wait before piggybacking a blacklist
     * update again after a failure (in ms)
     */
    simtime_t PIGGYBACKING_BACKOFF;

    /**
     * The "time to live" of the pkt: simtime (not hops!!) after which the pkt
     * is considered stale and will be dropped from any tx queue.
     * This is mandated by the way WAIC works, not by 6P.
     */
    simtime_t TxQueueTTL;

    // transaction stats
    int numTimeouts;
    int numConcurrentTransactionErrors;
    int numUnexpectedResponses;
    int numDuplicateResponses;
    int numExpiredReq;
    int numExpiredRsp;
    int numClearReqReceived;
    int numResetsReceived;
    int numOverlappingRequests;

    /** Data to be piggybacked (if any), indexed by destination. */
    std::map<uint64_t, std::vector<tsch6pPiggybackTimeoutMsg*>> piggybackableData;

    /** Control message waiting to be sent to the lower layer, should the LL ACK
        confirming the success of the transaction that caused the pattern update
        arrive in time. Indexed by destination: for each neighbor node, one
        update can be store. */
    std::map<uint64_t, tsch6topCtrlMsg*> pendingPatternUpdates;

    /* gates to MAC layer */
    GateId linkInfoControlIn;
    GateId lowerLayerOut;
    GateId lowerLayerIn;
    //GateId lowerControlOut;
    GateId lowerControlIn;

    /* the ID of this node, derived from its MAC address */
    uint64_t pNodeId;

    /** Information about all links maintained by this node */
    TschLinkInfo *pTschLinkInfo;

    /** The Scheduling Function used by this 6top sublayer */
    TschSF *pTschSF;

    /** Identifier of pTschSF. */
    tsch6pSFID_t pSFID;

    /** The time at which the SF actively starts sending requests */
    simtime_t pSFStarttime;

    /** statistics: number of 6p messages sent */
    simsignal_t s_6pMsgSent;

    simsignal_t sent6pClearSignal;
    simsignal_t sent6pAddSignal;
    simsignal_t sent6pRelocateSignal;
    simsignal_t sent6pDeleteSignal;
    simsignal_t sent6pResponseSignal;

    /** stats: number of 6p messages ACKed */
    simsignal_t acked6pAddSignal;
    simsignal_t acked6pRelocateSignal;
    simsignal_t acked6pDeleteSignal;
    simsignal_t acked6pResponseSignal;

    enum tsch6topSelfMessage_t {
        SF_START,
        PIGGYBACK_TIMEOUT
    };

    /**
     * @brief Handle a 6p message (i.e. perform basic checks, then hand it over
     *        to @ref handleRequestMsg() or @ref handleResponseMsg())
     */
    Packet* handle6PMsg(Packet* pkt);

    /**
     * @brief Handle 6p message @p pkt of type MSG_REQUEST.
     */
    Packet* handleRequestMsg(Packet* pkt, inet::IntrusivePtr<const tsch::sixtisch::SixpHeader>& hdr, inet::IntrusivePtr<const tsch::sixtisch::SixpData>& data);

    /**
     * Handle link-layer ACK received for the last 6P request
     *
     * @param destId id of the destination-originator of the ACK
     * @param cmd 6top command type - ADD/DELETE/RELOCATE
     */
    void handleRequestAck(uint64_t destId, tsch6pCmd_t cmd);

    /**
     * @brief Handle 6p packet @p pkt of type MSG_RESPONSE.
     */
    Packet* handleResponseMsg(Packet* pkt, inet::IntrusivePtr<const tsch::sixtisch::SixpHeader>& hdr, inet::IntrusivePtr<const tsch::sixtisch::SixpData>& data);

    /**
     * @brief Handle the control message that notifies this sublayer whether
     *        its last transmission was successful (i.e. whether we received a
     *        LL ACK or not).
     *        This is important for 6P transactions that were
     *        not initiated by this node: only after we've received the LL ACK,
     *        we can actually add/delete/relocate the cells that were agreed on
     *        in the current transaction!
     *
     * @return             Control message that was created in response to
     *                     @p msg. Id != NULL, it MUST be passed on to the lower
     *                     layer
     */
    void receiveSignal(cComponent *source, simsignal_t signalID, cObject *value, cObject *details) override;

    /**
     * @brief Handle a timeout of the transaction with @p destId (if any).
     *        Note that this appears to be the task of the Scheduling Function
     *        (at least SFX defines how to handle such a situation, and the 6P
     *        draft doesn't), but that makes no sense to me, so I'm violating
     *        those (inferred) boundaries.
     *
     * @param              The timeout notification that triggered this method
     */
    Packet* handleTransactionTimeout(tschLinkInfoTimeoutMsg* tom);

    /**
     * @brief Create a 6P Add request. Note that this does *not* send the request.
     *
     * @param destId       Id of the node to which this request will be sent
     * @param seqNum       The sequence number for this transaction
     * @param cellOptions  The CellOptions as described in the 6P draft
     * @param numCells     The number of cells that are requested for allocation
     * @param cellList     Cells proposed for allocation (must have length
     *                     >= @param numCells)
     * @param timeout      The time after which a response is expected at the
     *                     latest in ms. Note that this is the *absolute*
     *                     time, i.e. "at 23 minutes and 42 simulation seconds".
     *
     * @return             The created packet on success
     *                     NULL otherwise
     */
    Packet* createAddRequest(uint64_t destId, uint8_t seqNum, uint8_t cellOptions,
                            int numCells, std::vector<cellLocation_t> &cellList,
                            simtime_t timeout);

    /**
     * @brief Create a 6P Delete request. Note that this does *not* send the request.
     *
     * @param destId       Id of the node to which this request will be sent
     * @param seqNum       The sequence number for this transaction
     * @param cellOptions  The CellOptions as described in the 6P draft
     * @param numCells     The number of cells that are requested for deletion
     * @param cellList     Cells proposed for deletion (must have length
     *                     >= @param numCells)
     * @param timeout      The time after which a response is expected at the
     *                     latest in ms. Note that this is the *absolute*
     *                     time, i.e. "at 23 minutes and 42 simulation seconds".
     *
     * @return             The created packet on success
     *                     NULL otherwise
     */
    Packet* createDeleteRequest(uint64_t destId, uint8_t seqNum, uint8_t cellOptions,
                            int numCells, std::vector<cellLocation_t> cellList,
                            simtime_t timeout);

    /**
     * @brief Create a 6P Relocation request. Note that this does *not* send
     *        the request.
     * @param destId       Id of the node to which this request will be sent
     * @param seqNum       The sequence number for this transaction
     * @param cellOptions  The CellOptions as described in the 6P draft
     * @param numCells     The number of cells that are requested for relocation
     * @param relocationCellList The cells to be relocated
     * @param candidateCellList  The proposed alternate cells
     * @param timeout      The time after which a response is expected at the
     *                     latest in ms. Note that this is the *absolute*
     *                     time, i.e. "at 23 minutes and 42 simulation seconds".
     *
     * @return             The created packet on success
     *                     NULL otherwise
     */
    Packet* createRelocationRequest(uint64_t destId, uint8_t seqNum, uint8_t cellOptions,
                            int numCells, std::vector<cellLocation_t> &relocationCellList,
                            std::vector<cellLocation_t> &candidateCellList,
                            simtime_t timeout);

    /**
     * @brief Create a 6P Clear request. Note that this does *not* send
     *        the request.
     *
     * @param destId       Id of the node to which this request will be sent
     * @param seqNum       The sequence number for this transaction
     *
     * @return             The created packet on success
     *                     NULL otherwise
     */
    Packet* createClearRequest(uint64_t destId, uint8_t seqNum);


    /**
     * @brief Create a 6P Signal request. Note that this does *not* send
     *        the request.
     *
     * @param destId       Id of the node to which this request will be sent
     * @param seqNum       The sequence number for this transaction
     * @param payload      The payload of this signal message
     * @param payloadSz    The size the payload would have in the "real" packet
     *                     (in Bits)
     *
     * @return             The created packet on success
     *                     NULL otherwise
     */
    Packet* createSignalRequest(uint64_t destId, uint8_t seqNum, void* payload,
                                      int payloadSz);

    /**
     * @brief Create a 6P Success Response indicating that cells have been allocated
     *        (partially) successfully.
     *        Note that this does *not* send the response.
     *
     * @param destId       Id of the node to which this response will be sent
     * @param seqNum       The sequence number of the request that triggered
     *                     this response
     * @param cellList     The cells that have been successfully allocated
     */
    Packet* createSuccessResponse(uint64_t destId, uint8_t seqNum,
                                        std::vector<cellLocation_t> &cellList,
                                        simtime_t timeout);

    /**
     * @brief Create a 6P Error Response of any of the
     *        return code types specified in @ref tsch6pReturn_t, except for
     *        RC_SEQNUM, RC_SUCCESS and RC_EOL.
     *        Note that this function does *not* send the response.
     *
     * @param destId       Id of the node to which this response will be sent
     * @param seqNum       The sequence number of the request that triggered
     *                     this response
     * @param returnCode   The Return Code to be set
     * @return             The requested error response packet on success,
     *                     NULL on failure.
     */
    Packet* createErrorResponse(uint64_t destId, uint8_t seqNum,
                                      tsch6pReturn_t returnCode, simtime_t timeout);

    /**
     * @brief Create a 6P RC_SEQNUM Error Response.
     *
     * @param destId       Id of the node to which this response will be sent
     * @param seqNum       The sequence number which triggered this response
     * @return             The requested error response packet on success,
     *                     NULL on failure.
     */
    Packet* createSeqNumErrorResponse(uint64_t destId, uint8_t seqNum,
                                            simtime_t timeout);

    /**
     * @brief Create a 6P Response to a CLEAR Request.
     *
     * @param destId       Id of the node to which this response will be sent
     * @param seqNum       The sequence number of the request that triggered
     *                     this response
     * @param returnCode   The Return Code to be set (MUST be RC_SUCCESS or RC_RESET)
     */
    Packet* createClearResponse(uint64_t destId, uint8_t seqNum,
                                      tsch6pReturn_t returnCode,
                                      simtime_t timeout);

    /**
     * @brief Configure a tsch6topCtrlMsg of type CTRLMSG_PATTERNUPDATE.
     *        Each pattern update concerns the links to only one neighbor,
     *        i.e. all cells in @p cellList MUST belong to a link to @p destId.
     *        Note that this function does *not* send the update.
     *
     * @param msg          The message to be configured
     * @param destId       The neighbor affected by the pattern update
     * @param cellOption   The linkOption of all members of cellList
     * @param newCells     The cells to be added
     * @param deleteCells  The cells to be deleted
     * @param timeout      The absolute time at which this update becomes invalid
     */
    tsch6topCtrlMsg* setCtrlMsg_PatternUpdate(
                            tsch6topCtrlMsg* msg,
                            uint64_t destId,
                            uint8_t cellOption,
                            std::vector<cellLocation_t> newCells,
                            std::vector<cellLocation_t> deleteCells,
                            simtime_t timeout);

    /**
     * @return             true if none at all or more than one type of linkOption
     *                     (RX, TX or SHARED) is set in @p cellOptions
     *                     false otherwise
     */
    bool getCellOptions_isMultiple(uint8_t cellOptions);

    /**
     * @brief Get the link option set in @p cellOptions: Note that this only works
     *        correctly if only *one* option is set; remember to check
     *        @ref getCellOptions_isMultiple() first!
     */
    uint8_t getCellOption(uint8_t cellOptions);

    /**
     * @brief Adjust @p senderOpt for the receiver of a 6P Request:
     *        A Tx cell for sender is an Rx cell from the receivers' perspective
     *
     * @return         MAC_LINKOPTIONS_TX if @p senderOpt was MAC_LINKOPTIONS_RX
     *                 MAC_LINKOPTIONS_RX if @p senderOpt was MAC_LINKOPTIONS_TX
     *                 MAC_LINKOPTIONS_SHARED otherwise
     */
    uint8_t invertCellOption(uint8_t senderOpt);

    /**
     * @brief Calculate the point in time "now + timeoutOffset".
     *
     * @param timeoutOffset    The relative timeout in ms.
     * @return                 The absolute timeout
     */
    simtime_t getAbsoluteTimeout(int timeoutOffset);

    /**
     * @brief Update internal information about the transactions and links to
     *        @p destId to "we're about to send a request" state.
     *        If a link with @p destId exists, update the linkinfo. If no link
     *        exists yet, register linkinfo that we can fill as soon as the link
     *        has been established.
     *
     * @return             The current sequence number for this link
     */
    uint8_t prepLinkForRequest(uint64_t destId, simtime_t absoluteTimeout);

    /**
     * @brief If there's any Data that can be piggybacked to @p pkt,
     *        Add it to @p pkt.
     */
    void piggybackOnMessage(Packet* pkt, uint64_t destId);

    /**
     * @brief Delete entry from @ref piggybackableData where
     *        entry.payload == @p payloadPtr
     *
     * @param destId       The node the data was supposed to be piggybacked to
     */
    void deletePiggybackableData(uint64_t destId, void* payloadPtr);

    /**
     * @brief Send @p msg to its destination (via MAC, PHY etc)
     */
    void sendMessageToRadio(cMessage *msg) { sendMessageToRadio(msg, 0); } ;
    void sendMessageToRadio(cMessage *msg, double delay);

    bool hasPatternUpdateFor(uint64_t nodeId);

    /**
     * @brief Send control message to the MAC layer.
     */
    //void sendControlDown(cMessage *msg);


};

#endif /*__WAIC_TSCHNETLAYER_H_*/
