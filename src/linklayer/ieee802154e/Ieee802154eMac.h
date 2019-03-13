/* -*- mode:c++ -*- ********************************************************
 * file:        Ieee802154eMac.h
 *
 * author:     Jerome Rousselot, Marcel Steine, Amre El-Hoiydi,
 *                Marc Loebbers, Yosia Hadisusanto
 *
 * copyright:    (C) 2007-2009 CSEM SA
 *              (C) 2009 T.U. Eindhoven
 *                (C) 2004 Telecommunication Networks Group (TKN) at
 *              Technische Universitaet Berlin, Germany.
 *
 *              This program is free software; you can redistribute it
 *              and/or modify it under the terms of the GNU General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later
 *              version.
 *              For further information see file COPYING
 *              in the top level directory
 *
 * Funding: This work was partially financed by the European Commission under the
 * Framework 6 IST Project "Wirelessly Accessible Sensor Populations"
 * (WASP) under contract IST-034963.
 ***************************************************************************
 * part of:    Modifications to the MF-2 framework by CSEM
 **************************************************************************/

#ifndef __802154e_TSCH_H
#define __802154e_TSCH_H

#include "inet/linklayer/base/MacProtocolBase.h"
#include "inet/linklayer/common/MacAddress.h"
#include "inet/linklayer/contract/IMacProtocol.h"
#include "inet/physicallayer/contract/packetlevel/IRadio.h"
#include "TschSlotframe.h"
#include "Ieee802154eASN.h"
#include "TschHopping.h"

using namespace inet;

namespace tsch {

/**
 * @brief TSCH Mac-Layer.
 *
 * Supports constant, linear and exponential backoffs as well as
 * MAC ACKs.
 *
 * @author Jerome Rousselot, Amre El-Hoiydi, Marc Loebbers, Yosia Hadisusanto, Andreas Koepke
 * @author Karl Wessel (port for MiXiM)
 *
 * \image html csmaFSM.png "CSMA Mac-Layer - finite state machine"
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
        , sifs()
        , macAckWaitDuration()
        , headerLength(0)
        , transmissionAttemptInterruptedByRx(false)
        , ccaDetectionTime()
        , rxSetupTime()
        , aTurnaroundTime()
        , channelSwitchingTime()
        , macMaxFrameRetries(0)
        , useMACAcks(false)
        , backoffMethod(EXPONENTIAL)
        , macMinBE(0)
        , macMaxBE(0)
        , txPower(0)
        , NB(0)
        , macQueue()
        , queueLength(0)
        , txAttempts(0)
        , bitrate(0)
        , ackLength(0)
        , ackMessage(nullptr)
        , SeqNrParent()
        , SeqNrChild()
    {}

    virtual ~Ieee802154eMac();

    /** @brief Initialization of the module and some variables*/
    virtual void initialize(int) override;

    /** @brief Delete all dynamically allocated objects of the module*/
    virtual void finish() override;

    /** @brief Handle messages from lower layer */
    virtual void handleLowerPacket(inet::Packet *packet) override;

    /** @brief Handle messages from upper layer */
    virtual void handleUpperPacket(inet::Packet *packet) override;

    /** @brief Handle self messages such as timers */
    virtual void handleSelfMessage(omnetpp::cMessage *) override;

    /** @brief Handle control messages from lower layer */
    virtual void receiveSignal(cComponent *source, inet::simsignal_t signalID, long value, cObject *details) override;

    // OperationalBase:
    virtual void handleStartOperation(inet::LifecycleOperation *operation) override {}    //TODO implementation
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override {}    //TODO implementation
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override {}    //TODO implementation

  protected:
    typedef std::list<inet::Packet *> MacQueue;

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

    /*************************************************************/
    /****************** TYPES ************************************/
    /*************************************************************/

    /** @brief Kinds for timer messages.*/
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
        EV_TIMER_SLOTEND
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

    /** @brief keep track of MAC state */
    t_mac_states macState;
    t_mac_status status;

    /** @brief The radio. */
    inet::physicallayer::IRadio *radio;
    inet::physicallayer::IRadio::TransmissionState transmissionState;

    /** @brief Maximum time between a packet and its ACK
     *
     * Usually this is slightly more then the tx-rx turnaround time
     * The channel should stay clear within this period of time.
     */
    omnetpp::simtime_t sifs;

    /** @brief timeslot length
     *
     * "The total length of the timeslot including any unused time after
     *  frame transmission and acknowledgment, in us"
     */
    omnetpp::simtime_t macTsTimeslotLength;

    bool useCCA;

    /** @brief The amount of time the MAC waits for the ACK of a packet.*/
    omnetpp::simtime_t macAckWaitDuration;

    /** @brief Length of the header*/
    int headerLength;

    bool transmissionAttemptInterruptedByRx;
    /** @brief CCA detection time */
    omnetpp::simtime_t ccaDetectionTime;
    /** @brief Time to setup radio from sleep to Rx state */
    omnetpp::simtime_t rxSetupTime;
    /** @brief Time to switch radio from Rx to Tx state */
    omnetpp::simtime_t aTurnaroundTime;
    omnetpp::simtime_t channelSwitchingTime;
    /** @brief maximum number of frame retransmissions without ack */
    unsigned int macMaxFrameRetries;
    /** @brief Stores if the MAC expects Acks for Unicast packets.*/
    bool useMACAcks;

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

    /** @brief A queue to store packets from upper layer in case another
       packet is still waiting for transmission..*/
    MacQueue macQueue;

    /** @brief length of the queue*/
    unsigned int queueLength;

    /** @brief count the number of tx attempts
     *
     * This holds the number of transmission attempts for the current frame.
     */
    unsigned int txAttempts;

    /** @brief the bit rate at which we transmit */
    double bitrate;

    /** @brief The bit length of the ACK packet.*/
    int ackLength;



    TschSlotframe *sf;
    Ieee802154eASN asn;

  protected:
    /** @brief Generate new interface address*/
    virtual void configureInterfaceEntry() override;
    virtual void handleCommand(omnetpp::cMessage *msg) {}

    virtual void flushQueue();

    virtual void clearQueue();

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
//    void updateStatusNotIdle(omnetpp::cMessage *msg);
    void manageQueue();
    void updateMacState(t_mac_states newMacState);

    void attachSignal(inet::Packet *mac, omnetpp::simtime_t_cref startTime);
    void manageMissingAck(t_mac_event event, omnetpp::cMessage *msg);
    void startTimer(t_mac_timer timer);

    omnetpp::simtime_t scheduleSlot();

    virtual void decapsulate(inet::Packet *packet);

    inet::Packet *ackMessage;

    //sequence number for sending, map for the general case with more senders
    //also in initialisation phase multiple potential parents
    std::map<inet::MacAddress, unsigned long> SeqNrParent;    //parent -> sequence number

    //sequence numbers for receiving
    std::map<inet::MacAddress, unsigned long> SeqNrChild;    //child -> sequence number

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

