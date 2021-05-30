/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright   (C) 2021  Institute of Communication Networks (ComNets),
 *                       Hamburg University of Technology (TUHH)
 *                       Leo Krueger, Louis Yin
 *
 * This work is based on Ieee802154Mac.h from INET 4.1:
 *
 * Author:     Jerome Rousselot, Marcel Steine, Amre El-Hoiydi,
 *             Marc Loebbers, Yosia Hadisusanto, Andreas Koepke
 *
 * Copyright:  (C) 2009 T.U. Eindhoven
 *             (C) 2007-2009 CSEM SA
 *             (C) 2004,2005,2006
 *             Telecommunication Networks Group (TKN) at Technische
 *             Universitaet Berlin, Germany.
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

#ifndef __802154e_TSCH_H
#define __802154e_TSCH_H

#include "inet/linklayer/base/MacProtocolBase.h"
#include "inet/linklayer/common/MacAddress.h"
#include "inet/linklayer/contract/IMacProtocol.h"
#include "inet/physicallayer/contract/packetlevel/IRadio.h"
#include "inet/physicallayer/common/packetlevel/MediumLimitCache.h"
#include "inet/networklayer/common/InterfaceTable.h"
#include "TschSlotframe.h"
#include "Ieee802154eASN.h"
#include "TschHopping.h"
#include "TschNeighbor.h"
#include "inet/common/Units.h"
#include <vector>
#include <tuple>
#include "sixtisch/Tsch6tischComponents.h"

using namespace inet;
using namespace std;

namespace tsch {

/**
 * @brief TSCH Mac-Layer.
 *
 * \image html TSCH_CSMA.png "CSMA Mac-Layer"
 * \image html TSCH_FSM.png "TSCH finite state machine"
 */
class Ieee802154eMac : public inet::MacProtocolBase, public inet::IMacProtocol
{
  public:
    Ieee802154eMac()
        : MacProtocolBase()
        , nbTxFrames(0)
        , nbRxFrames(0)
        , nbMissedAcks(0)
        , nbRecvdAcks(0)
        , nbDroppedFrames(0)
        , nbTxAcks(0)
        , nbDuplicates(0)
        , nbBackoffs(0)
        , backoffValues(0)
        , slotTimer(nullptr), ccaTimer(nullptr), sifsTimer(nullptr), rxAckTimer(nullptr), slotendTimer(nullptr), hoppingTimer(nullptr)
        , macState(IDLE_1)
        , status(STATUS_OK)
        , radio(nullptr)
        , transmissionState(inet::physicallayer::IRadio::TRANSMISSION_STATE_UNDEFINED)
        //, sifs()
        , macTsTxAckDelay()
        //, macAckWaitDuration()
        , macTsRxAckDelay()
        , macTsAckWait()
        , macTsMaxAck()
        , headerLength(0)
        , transmissionAttemptInterruptedByRx(false)
        , ccaDetectionTime()
        , rxSetupTime()
        , macTsRxTx()
        , channelSwitchingTime()
        , macMaxFrameRetries(0)
        , useMACAcks(false)
        , backoffMethod(EXPONENTIAL)
        , txPower(0)
        , NB(0)
        , txAttempts(0)
        , bitrate(0)
        , ackLength(0)
        , ackMessage(nullptr)
        , SeqNrParent()
        , SeqNrChild()
        , pArtificialPacketDrop(false)
    {
    }

    /** @brief Gate ids */
    //@{
    int sixTopSublayerInGateId;
    int sixTopSublayerOutGateId;
    //int sixTopSublayerControlInGateId;
    int sixTopSublayerControlOutGateId;
    //@}
    virtual ~Ieee802154eMac();

    /** @brief Initialization of the module and some variables*/
    virtual void initialize(int) override;

    /** @brief Delete all dynamically allocated objects of the module*/
    virtual void finish() override;
    void finish(cComponent *component, simsignal_t signalID) override { cIListener::finish(component, signalID); };

    virtual bool isUpperMessage(cMessage *message) override;

    /** @brief Handle messages from lower layer */
    virtual void handleLowerPacket(inet::Packet *packet) override;

    /** @brief Handle messages from upper layer */
    virtual void handleUpperPacket(inet::Packet *packet) override;

    /** @brief Handle self messages such as timers */
    virtual void handleSelfMessage(omnetpp::cMessage *) override;

    /** @brief Handle control messages from lower layer */
    virtual void receiveSignal(cComponent *source, inet::simsignal_t signalID, long value, cObject *details) override;

    virtual inet::physicallayer::IRadio* getRadio();

    void sendUp(cMessage *message) override;

    InterfaceEntry *getInterfaceEntry();

    // OperationalBase:
    virtual void handleStartOperation(inet::LifecycleOperation *operation) override {}    //TODO implementation
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override {}    //TODO implementation
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override {}    //TODO implementation


    // Utility
    list<uint64_t> getNeighborsInRange(); // get list of neighbors based on maximum communication range

    vector<tuple<int, int>> getQueueSizes(MacAddress neighbor, vector<int> virtualLinkIds = {-1, 0});

    /**
     * Compute queue utilization as number of packets in queue / queue size
     *
     * @param neighbor neighbor MAC address to compute the queue size for
     *
     * @return queue utilization
     */
    double getQueueUtilization(MacAddress neighbor);

    void enableArtificialPacketDrop() { this->pArtificialPacketDrop = true; }
    void disableArtificialPacketDrop() { this->pArtificialPacketDrop = false; }

  protected:
    /** @name Different tracked statistics.*/
    /*@{*/
    long nbTxFrames;
    long nbRxFrames;
    long nbMissedAcks;
    long nbRecvdAcks;
    long nbDroppedFrames;
    long nbTxAcks;
    long nbDuplicates;
    long nbBackoffs;
    double backoffValues;
    /*@}*/

    /** @brief MAC states
     * see states diagram.
     */
    enum t_mac_states {
        IDLE_1 = 1,
        HOPPING_2 = 2,
        CCA_3,
        TRANSMITFRAME_4,
        WAITACK_5,
        RECEIVEFRAME_6,
        WAITSIFS_7,
        TRANSMITACK_8
    };

    friend std::ostream& operator<<(std::ostream& out, const t_mac_states state) {
        const char* s = 0;
        switch(state) {
            PRINT_ENUM(IDLE_1, s);
            PRINT_ENUM(HOPPING_2, s);
            PRINT_ENUM(CCA_3, s);
            PRINT_ENUM(TRANSMITFRAME_4, s);
            PRINT_ENUM(WAITACK_5, s);
            PRINT_ENUM(RECEIVEFRAME_6, s);
            PRINT_ENUM(WAITSIFS_7, s);
            PRINT_ENUM(TRANSMITACK_8, s);
        }
        return out << s;
    }


    /*************************************************************/
    /****************** TYPES ************************************/
    /*************************************************************/

    /** @brief Kinds of timer messages.*/
    enum t_mac_timer {
        TIMER_NULL = 0,
        TIMER_SLOT,
        TIMER_CCA,
        TIMER_SIFS,
        TIMER_RX_ACK,
        TIMER_HOPPING,
        TIMER_SLOTEND
    };

    /** @name Pointer for timer messages.*/
    /*@{*/
    omnetpp::cMessage *slotTimer, *ccaTimer, *sifsTimer, *rxAckTimer, *slotendTimer, *hoppingTimer;
    /*@}*/

    /** @brief MAC state machine events.
     * See state diagram.*/
    enum t_mac_event {
        EV_SEND_REQUEST = 1,    // 1, 11, 20, 21, 22
        EV_TIMER_BACKOFF,    // 2, 7, 14, 15
        EV_FRAME_TRANSMITTED,    // 4, 19
        EV_ACK_RECEIVED,    // 5
        EV_ACK_TIMEOUT,    // 12
        EV_FRAME_RECEIVED,    // 15, 26
        EV_DUPLICATE_RECEIVED,
        EV_TIMER_SIFS,    // 17
        EV_BROADCAST_RECEIVED,    // 23, 24
        EV_TIMER_CCA,
        EV_TIMER_SLOT,
        EV_TIMER_HOPPING,
        EV_TIMER_SLOTEND,
        MAC_ENABLE_DROPS // TEST EVENT, not part of the normal MAC operation, enable packet drops after a timeout
    };

    /** @brief Types for frames sent by the CSMA.*/
    enum t_csma_frame_types {
        DATA,
        ACK
    };

    enum t_mac_carrier_sensed {
        CHANNEL_BUSY = 1,
        CHANNEL_FREE
    };

    enum t_mac_status {
        STATUS_OK = 1,
        STATUS_ERROR,
        STATUS_RX_ERROR,
        STATUS_RX_TIMEOUT,
        STATUS_FRAME_TO_PROCESS,
        STATUS_NO_FRAME_TO_PROCESS,
        STATUS_FRAME_TRANSMITTED
    };

    /** @brief The different back-off methods.*/
    enum backoff_methods {
        /** @brief Constant back-off time.*/
        CONSTANT = 0,
        /** @brief Linear increasing back-off time.*/
        LINEAR,
        /** @brief Exponentially increasing back-off time.*/
        EXPONENTIAL,
    };

    enum signal_names {
        NBTXFRAMES = 0,
        NBMISSEDACKS,
        NBRECVDACKS,
        NBRXFRAMES,
        NBTXACKS,
        NBDUPLICATES,
        NBSLOT
    };

    /** @brief keep track of MAC state */
    t_mac_states macState;
    t_mac_status status;

    /** @brief The radio. */
    inet::physicallayer::IRadio *radio;
    inet::physicallayer::IRadio::TransmissionState transmissionState;

    TschHopping *hopping;

    /** @brief Maximum time between a packet and its ACK
     *
     * Usually this is slightly more then the tx-rx turnaround time
     * The channel should stay clear within this period of time.
     */
    //omnetpp::simtime_t sifs;
    omnetpp::simtime_t macTsTxAckDelay;

    /** @brief timeslot length
     *
     * "The total length of the timeslot including any unused time after
     *  frame transmission and acknowledgment, in us"
     */
    omnetpp::simtime_t macTsTimeslotLength;

    bool useCCA;

    /** @brief The amount of time the MAC waits for the ACK of a packet.*/
    //omnetpp::simtime_t macAckWaitDuration;
    /** @brief The amount of offset time the Tx MAC waits for the ACK of a packet.*/
    omnetpp::simtime_t macTsRxAckDelay;
    /** @brief The amount of time the MAC waits for the ACK of a packet.*/
    omnetpp::simtime_t macTsAckWait;
    /** @brief The amount of time the ACK needs to be transmitted completly.*/
    omnetpp::simtime_t macTsMaxAck;

    /** @brief Length of the header*/
    int headerLength;

    bool transmissionAttemptInterruptedByRx;
    /** @brief CCA detection time */
    omnetpp::simtime_t ccaDetectionTime;
    /** @brief Time to setup radio from sleep to Rx state */
    omnetpp::simtime_t rxSetupTime;
    /** @brief Time to switch radio from Rx to Tx state */
    omnetpp::simtime_t macTsRxTx;
    omnetpp::simtime_t channelSwitchingTime;
    /** @brief maximum number of frame retransmissions without ack */
    unsigned int macMaxFrameRetries;
    /** @brief Stores if the MAC expects Acks for Unicast packets.*/
    bool useMACAcks;

    // Prioritize dedicated TX cells used for application traffic in case several
    // cells are scheduled at the same slot offset
    bool pPrioAppData;

    /** @brief Defines the backoff method to be used.*/
    backoff_methods backoffMethod;

    /**
     * @brief Minimum backoff exponent.
     * Only used for exponential backoff method.
     */
    int macMinBE;
    /**
     * @brief Maximum backoff exponent.
     * Only used for exponential backoff method.
     */
    int macMaxBE;

    /** @brief The power (in mW) to transmit with.*/
    double txPower;

    /** @brief number of backoff performed until now for current frame */
    int NB;

    /** @brief count the number of tx attempts
     *
     * This holds the number of transmission attempts for the current frame.
     */
    unsigned int txAttempts;

    /** @brief the bit rate at which we transmit */
    double bitrate;

    /** @brief The bit length of the ACK packet.*/
    int ackLength;

    cProperty *statisticTemplate;

    std::vector<simsignal_t> channelSignals;
    simsignal_t pktEnqueuedSignal;

    std::vector<std::string> registeredSignals;

    TschNeighbor *neighbor;
    TschSlotframe *schedule;
    Ieee802154eASN asn;

    int64_t currentAsn;
    TschLink *currentLink;
    int currentChannel;

    bool pArtificialPacketDrop;
    double pPacketDropThreshold;

  protected:
    /** @brief Generate new interface address*/
    virtual void configureInterfaceEntry() override;
    virtual void handleCommand(omnetpp::cMessage *msg) {}

    virtual void flushQueue();

    virtual void emitSignal(signal_names signalName);

    /** @brief Asynchronously configure carrier frequency and mode of radio
     *
     * When a default value is given for one of the parameters,
     * radio is left unchanged in regard to that parameter.
     *
     * @note does not generate signals on which this MAC relies at some points,
     * so only use when you are sure about it
     */
    void configureRadio(Hz carrierFrequency = Hz(NAN), int mode = -1);

    // FSM functions
    void fsmError(t_mac_event event, omnetpp::cMessage *msg);
    void executeMac(t_mac_event event, omnetpp::cMessage *msg);
    void updateStatusIdle(t_mac_event event, omnetpp::cMessage *msg);
    void updateStatusHopping(t_mac_event event, omnetpp::cMessage *msg);
    void updateStatusCCA(t_mac_event event, omnetpp::cMessage *msg);
    void updateStatusTransmitFrame(t_mac_event event, omnetpp::cMessage *msg);
    void updateStatusWaitAck(t_mac_event event, omnetpp::cMessage *msg);
    void updateStatusReceiveFrame(t_mac_event event, omnetpp::cMessage *msg);
    void updateStatusSIFS(t_mac_event event, omnetpp::cMessage *msg);
    void updateStatusTransmitAck(t_mac_event event, omnetpp::cMessage *msg);
    void updateMacState(t_mac_states newMacState);

    void attachSignal(inet::Packet *mac, omnetpp::simtime_t_cref startTime);
    void manageMissingAck(t_mac_event event, omnetpp::cMessage *msg);
    void manageFailedTX();
    void startTimer(t_mac_timer timer);

    omnetpp::simtime_t scheduleSlot();

    int getVirtualLinkId(TschLink* link);

    /**
     * --Yevhenii
     *
     * Custom functions to handle cell overlapping in time.
     *
     * Select current active link by iterating through all links / cells
     * and applying the following "priorities", rather than selecting the first one found:
     *  1. Dedicated TX link with non-empty queue (if @param prioAppData is true)
     *  2. Shared TX link with non-empty queue
     *  3. Random RX link
     *
     *  @param links list of all links scheduled with neigbhors
     *  @return link selected to be active for current ASN
     */
    TschLink* selectActiveLink(std::vector<TschLink*> links);
    TschLink* selectActiveLink(std::vector<TschLink*> links, bool prioAppData);

    virtual void decapsulate(inet::Packet *packet);

    inet::Packet *ackMessage;

    //sequence number for sending, map for the general case with more senders
    //also in initialisation phase multiple potential parents
    std::map<inet::MacAddress, std::map<int, unsigned long>> SeqNrParent;    //parent -> sequence number

    //sequence numbers for receiving
    std::map<inet::MacAddress, std::map<int, unsigned long>> SeqNrChild;    //child -> sequence number

    // Util
    list<MacAddress> neighbors; // list of neighbors MAC addresses
    bool artificialPacketDrop();

  private:
    /** @brief Copy constructor is not allowed.
     */
    Ieee802154eMac(const Ieee802154eMac&);
    /** @brief Assignment operator is not allowed.
     */
    Ieee802154eMac& operator=(const Ieee802154eMac&);
};
}

#endif // ifndef __802154e_TSCH_H

