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

simple TschCSMA
{
    parameters:
        int NB = default(-1);
        int BE = default(-1);
        int randomNumber = default(-1);
        @display("i=block/table");
        @class(TschCSMA);
}