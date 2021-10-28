// 
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2021  Institute of Communication Networks (ComNets), 
//                     Hamburg University of Technology (TUHH)
//           (C) 2019  Leo Krueger
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

@namespace(tsch);

simple TschHopping
{
    parameters:
        int nbRadioChannels = default(0); // TODO: move this elsewhere to radio medium
        string pattern = default("11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26"); // ramp
        bool useRandomPattern = default(true);
//      string pattern = default("16 17 23 18 26 15 25 22 19 11 12 13 24 14 20 21"); // from contiki
//      string pattern = default("11"); 											 // hopping disabled
		double centerFrequency @unit(Hz) = default (2405 MHz); 
        @display("i=block/table");
        @class(TschHopping);
}

