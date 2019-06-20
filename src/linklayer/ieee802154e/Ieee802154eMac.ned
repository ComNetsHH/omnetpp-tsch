// 
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2019  Institute of Communication Networks (ComNets), 
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
        double aTurnaroundTime @unit(s) = default(0.000192 s);    // 12 symbols
        double channelSwitchingTime @unit(s) = default(0.000192 s); // 12 symbols TODO put reasonable value
        // Send/Expect MAC acks for unicast traffic?
        bool useMACAcks = default(true);
        // Maximum number of frame retransmission,
        // only used when usage of MAC acks is enabled.
        int macMaxFrameRetries = default(3);
        // Time to wait for an acknowledgement after transmitting a unicast frame.
        // Only used when usage of MAC acks is enabled.
        // Value is calculated from 1+12+10+12 symbols, which is defined for nonbeacon-enabled PAN.
        // In the non-beacon-enabled case, the receiver responds at aTurnaroundTime
        // (i.e. the time for the sender and receiver to both be guaranteed to have
        // switched from Tx to Rx and vice verse).  This gives the value 192us +
        // 352us = 544us (there's been some discussion about the "extra" 1 symbol == 16us)
        // [section 7.5.6.4.2 of the specification].
        double macAckWaitDuration @unit(s) = default(0.00056 s);
        // Complete MAC ack message length (in bits)
        // (! headerLength is not added to this),
        // only used when usage of MAC acks is enabled.
        int ackLength @unit(b) = default(40 b);
        // Simple interframe space (12 symbols). Time to wait between receiving a frame and acknowledging it.
        // Should be bigger than the maximum time for switching between Tx and Rx at the receiver.
        // Only used when usage of MAC acks is enabled.
        double sifs @unit(s) = default(0.000192 s);
		// "The total length of the timeslot including any unused time after
		//  frame transmission and acknowledgment, in us"
		double macTsTimeslotLength @unit(s) = default(0.01 s);
		bool useCCA = default(false);
        //Backoff method to use: constant, linear or exponential
        // "The device shall use an exponential backoff mechanism [..]"
        string backoffMethod = default("exponential");
        // The path to the Radio module  //FIXME remove default value
        string radioModule = default("^.^.radio");

        @class(Ieee802154eMac);
        @signal[linkBroken](type=inet::Packet);
        @statistic[linkBroken](title="link break"; source=linkBroken; record=count; interpolationmode=none);
        @statistic[packetDropNotAddressToUs](title="packet drop: not addressed to us"; source=packetDropReasonIsNotAddressedToUs(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropIncorrectlyReceived](title="packet drop: incorrectly received"; source=packetDropReasonIsIncorrectlyReceived(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropQueueOverlow](title="packet drop: queue overflow"; source=packetDropReasonIsQueueOverflow(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropRetryLimitReached](title="packet drop: retry limit reached"; source=packetDropReasonIsRetryLimitReached(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropBackoffLimitReached](title="packet drop: backoff limit reached"; source=packetDropReasonIsCongestion(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
}

