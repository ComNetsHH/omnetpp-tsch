// 
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2019  Institute of Communication Networks (ComNets), 
//                     Hamburg University of Technology (TUHH)
//           (C) 2019  Louis Yin
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

simple TschNeighbor
{
    parameters:
        bool hasPriorityQueue = default(false);
        //First Neighbor= 1,
        //Longest Queue = 2,
        string method = default("First");
        int queueLength = default(20);
        int macMinBE = default(3);
        int macMaxBE = default(8); 
		@display("i=block/table");
		// TODO add signals when neighbors are added to the Vector
		@class(TschNeighbor);
        
}
