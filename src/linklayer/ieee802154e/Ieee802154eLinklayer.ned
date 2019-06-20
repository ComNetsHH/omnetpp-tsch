// 
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2019  Institute of Communication Networks (ComNets), 
//                     Hamburg University of Technology (TUHH)
//           (C) 2019  Leo Krueger, Louis Yin
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
import tsch.linklayer.ieee802154e.TschSlotframe;
import tsch.linklayer.ieee802154e.TschHopping;
import tsch.linklayer.ieee802154e.TschNeighbor;
import tsch.linklayer.ieee802154e.Ieee802154eMac;

module Ieee802154eLinklayer like IMacProtocol
{
	gates:
        input upperLayerIn @labels(INetworkHeader/down);
        output upperLayerOut @labels(INetworkHeader/up);
        input lowerLayerIn @labels(ILinkLayerFrame/up);
        output lowerLayerOut @labels(ILinkLayerFrame/down);
    submodules:
        schedule: TschSlotframe;
        neighbor: TschNeighbor;
        channelHopping: TschHopping;
        mac: Ieee802154eMac;
	connections:
	    mac.lowerLayerIn <-- lowerLayerIn;
	    mac.lowerLayerOut --> lowerLayerOut;
	    mac.upperLayerIn <-- upperLayerIn;
	    mac.upperLayerOut --> upperLayerOut;
}
