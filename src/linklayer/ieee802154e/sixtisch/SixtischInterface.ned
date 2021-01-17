//
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2019  Institute of Communication Networks (ComNets),
//                     Hamburg University of Technology (TUHH)
//           (C) 2017  Lotte Steenbrink
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

package tsch.linklayer.ieee802154e.sixtisch;

import tsch.linklayer.ieee802154e.sixtisch.TschSF;
import tsch.linklayer.ieee802154e.sixtisch.TschMSF;
import tsch.linklayer.ieee802154e.sixtisch.TschSFSB;
import tsch.linklayer.ieee802154e.sixtisch.Tsch6TopSublayer;

module SixtischInterface
{
    parameters:
        bool stats = false;
        int nbRadioChannels = default(0);
        string sfid = default("TschMSF");
        @display("i=block/ifcard;bgb=750,443");
    gates:
        input lowerLayerIn;
        output lowerLayerOut;
        input lowerControlIn;
        //output lowerControlOut;

    submodules:
        sixtop: Tsch6TopSublayer {
            @display("p=400,100;i=block/layer");
        }
        sf: <default("TschMSF")> like TschSF {
            @display("p=600,100;i=block/layer");
        }
        linkinfo: TschLinkInfo {
            @display("p=100,200;i=block/layer");
        }
	    sensing: TschSpectrumSensing {
                @display("p=100,100;i=block/layer");
		}
    connections:
        linkinfo.sublayerControlOut --> sixtop.linkInfoControlIn;
        //        sensing.sublayerControlOut --> sixtop.sensingControlIn;
        //        sensing.sublayerControlIn <-- sixtop.sensingControlout;
        lowerLayerIn --> { @display("m=s"); } --> sixtop.lowerLayerIn;
        lowerLayerOut <-- { @display("m=s"); } <-- sixtop.lowerLayerOut;
        lowerControlIn --> { @display("m=s"); } --> sixtop.lowerControlIn;
        //lowerControlOut <-- sixtop.lowerControlOut;
}
