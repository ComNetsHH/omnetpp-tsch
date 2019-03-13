//
// Copyright (C) 2004 Andras Varga
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.

package tsch.linklayer.ieee802154e;

@namespace(tsch);

simple TschHopping
{
    parameters:
        string pattern = default("11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26"); // ramp
//      string pattern = default("16 17 23 18 26 15 25 22 19 11 12 13 24 14 20 21"); // from contiki
//      string pattern = default("11"); 											 // hopping disabled
        @display("i=block/table");
        @class(TschHopping);
}

