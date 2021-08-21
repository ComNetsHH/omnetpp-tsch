// 
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2021  Institute of Communication Networks (ComNets), 
//                     Hamburg University of Technology (TUHH)
//			 (C) 2019  Leo Krueger
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

import tsch.linklayer.contract.ISlotframe;

@namespace(tsch);

simple TschSlotframe like ISlotframe
{
    parameters:
        int macSlotframeSize = default(101); // recommended by IETF 6TiSCH Internet Draft
        int macSlotframeHandle = default(0);
        string fileName = default("Tsch_Schedule_example.xml")
        @display("i=block/table");
        @signal[linkAdded](type=tsch::TschLink);
        @signal[linkDeleted](type=tsch::TschLink);
        @signal[linkChanged](type=tsch::TschLink);
        @class(TschSlotframe);
}

