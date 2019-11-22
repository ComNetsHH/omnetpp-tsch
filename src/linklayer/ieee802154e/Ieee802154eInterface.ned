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
import inet.common.LayeredProtocolBase;
import inet.linklayer.contract.IMacProtocol;
import tsch.linklayer.ieee802154e.Ieee802154eMac;
import tsch.linklayer.ieee802154e.TschSlotframe;
import tsch.linklayer.ieee802154e.TschHopping;
import tsch.linklayer.ieee802154e.TschNeighbor;
import tsch.linklayer.ieee802154e.sixtisch.SixtischInterface;

module Ieee802154eInterface like IMacProtocol
{
    parameters:
        @display("i=block/ifcard");

    gates:
        input upperLayerIn @labels(INetworkHeader/down);
        output upperLayerOut @labels(INetworkHeader/up);
        input lowerLayerIn @labels(ILinkLayerFrame/up);
        output lowerLayerOut @labels(ILinkLayerFrame/down);

    submodules:
        mac: Ieee802154eMac {
            parameters:
                @display("p=100,200");
        }
        schedule: TschSlotframe {
            parameters:
                @display("p=300,200");
        }
        channelHopping: TschHopping {
            parameters:
                @display("p=300,300");
        }
        neighbor: TschNeighbor {
            parameters:
                @display("p=300,400");
        }
        sixtischInterface: SixtischInterface {
            parameters:
                @display("p=200,100");
        }
    connections:
        mac.lowerLayerIn <-- { @display("m=s"); } <-- lowerLayerIn;
        mac.lowerLayerOut --> { @display("m=s"); } --> lowerLayerOut;
        mac.upperLayerIn <-- { @display("m=n"); } <--  upperLayerIn;
        mac.upperLayerOut --> { @display("m=n"); }--> upperLayerOut;
        mac.sixTopSublayerOutGate --> sixtischInterface.lowerLayerIn;
        mac.sixTopSublayerInGate <-- sixtischInterface.lowerLayerOut;
        mac.sixTopSublayerControlOutGate --> sixtischInterface.lowerControlIn;
        //Ieee802154eMac.sixTopSublayerControlInGate <-- sixtischInterface.lowerControlOut;
}
