// 
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2021  Institute of Communication Networks (ComNets), 
//                     Hamburg University of Technology (TUHH)
//                     Leo Krueger, Louis Yin
//           
// This work is based on Ieee802154Mac.ned from INET 4.1:
//
// author:	 Jerome Rousselot, Marc Loebbers
//
// Copyright (C) 2007-2009 CSEM SA, Neuchatel, Switzerland.
//           (C) 2004 Telecommunication Networks Group (TKN) at
//*              Technische Universitaet Berlin, Germany.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

package tsch.linklayer.ieee802154e;

import inet.linklayer.base.MacProtocolBase;
import inet.linklayer.contract.IMacProtocol;

@namespace(tsch);

//
// Generic CSMA protocol supporting Mac-ACKs as well as
// constant, linear or exponential backoff times.
//
simple Ieee802154eMac extends MacProtocolBase like IMacProtocol
{
    parameters:
        // MAC address as hex string (12 hex digits), or
        // "auto". "auto" values will be replaced by
      	// a generated MAC address in init stage 0.
        string address = default("auto");
        // length of MAC header
        int headerLength @unit(b) = default(72 b);
        int mtu @unit(B) = default(0B);
        // bit rate
        double bitrate @unit(bps) = default(250000 bps);
        // Clear Channel Assessment detection time
        double ccaDetectionTime @unit(s) = default(0.000128 s); // 8 symbols
        // Time to setup radio to reception state
        double rxSetupTime @unit(s) = default(0 s); // TODO put reasonable value
        // Time to switch radio from Rx to Tx state
        // macTsRxTx
        double macTsRxTx @unit(s) = default(0.000192 s);    // 12 symbols
        // Is not used in an executable way
        double channelSwitchingTime @unit(s) = default(0.000192 s); // 12 symbols TODO put reasonable value
        // Send/Expect MAC acks for unicast traffic?
        bool useMACAcks = default(true);
        // Maximum number of frame retransmission,
        // only used when usage of MAC acks is enabled.
        int macMaxFrameRetries = default(3);
        // Time to wait before expecting an acknowledgement after transmitting a unicast frame.
        // In case power consumption needs to be researched the on/off of the TschNode needs to be adjusted
        double macTsRxAckDelay @unit(s) = default(0.0008 s);
        // Duration during which the acknowledgement is expected to start: macTsAckWait=400us.
        // For simulation, the value macTsMaxAck=160 us needs to be added, 
        // since the detection is done at the end of the fully transmitted packet
        double macTsAckWait @unit(s) = default(0.0004 s);
        // Maximum transmission duration of (Imm-)Ack (5 Octets)
        double macTsMaxAck @unit(s) = default(0.00016 s);
        // Complete MAC ack message length (in bits)
        // (! headerLength is not added to this),
        // only used when usage of MAC acks is enabled.
        int ackLength @unit(b) = default(40 b);
        // Simple interframe space (12 symbols). Time to wait between receiving a frame and acknowledging it.
        // Should be bigger than the maximum time for switching between Tx and Rx at the receiver.
        // Only used when usage of MAC acks is enabled.
        // sifs = macTsTxAckDelay
        //double sifs @unit(s) = default(0.000192 s);
        double macTsTxAckDelay @unit(s) = default(0.001 s);
		// "The total length of the timeslot including any unused time after
		//  frame transmission and acknowledgment, in us"
		double macTsTimeslotLength @unit(s) = default(0.01 s);
		bool useCCA = default(true);
        //Backoff method to use: constant, linear or exponential
        // "The device shall use an exponential backoff mechanism [..]"
        string backoffMethod = default("exponential");
        // The path to the Radio module  //FIXME remove default value
        string radioModule = default("^.^.radio");
        
        // See header explanation for pPrioAppData
        bool prioritizeApplicationData = default(false);
        
        // TEST parameters for aritifical packet drop scenarios
        // generating random number (0, 1) > this value will result in packet drop
        double packetDropThreshold = default(0.7);  
        // when MAC starts to apply artifical packet drops
        double lossyLinkTimeout @unit(s) = default(0s);

        @class(Ieee802154eMac);
        @signal[linkBroken](type=inet::Packet);
        @signal[queueUtilization](type=double);
        @statistic[linkBroken](title="link break"; source=linkBroken; record=count; interpolationmode=none);
        @statistic[packetDropNotAddressToUs](title="packet drop: not addressed to us"; source=packetDropReasonIsNotAddressedToUs(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropIncorrectlyReceived](title="packet drop: incorrectly received"; source=packetDropReasonIsIncorrectlyReceived(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropQueueOverlow](title="packet drop: queue overflow"; source=packetDropReasonIsQueueOverflow(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropRetryLimitReached](title="packet drop: retry limit reached"; source=packetDropReasonIsRetryLimitReached(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropBackoffLimitReached](title="packet drop: backoff limit reached"; source=packetDropReasonIsCongestion(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        
        @signal[nbTxFrames-*](type=long);
        @signal[nbMissedAcks-*](type=long);
        @signal[nbRecvdAcks-*](type=long);
        @signal[nbRxFrames-*](type=long);
        @signal[nbTxAcks-*](type=long);
        @signal[nbDuplicates-*](type=long);
        @signal[nbSlot-*](type=long);
        @signal[pktEnqueued](type=long);
        
        @statisticTemplate[nbStats](record=count,vector?; interpolationmode=none);
        
        int numSignals = default(0);
        
        gates:
            input sixTopSublayerInGate;
            output sixTopSublayerOutGate;
            //input sixTopSublayerControlInGate;
            output sixTopSublayerControlOutGate;
}

