//
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
// Self-messages for piggyback timeouts
//
// Copyright (C) 2021  Institute of Communication Networks (ComNets),
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

cplusplus {{
#include <stdint.h>
}}

message tsch6pPiggybackTimeoutMsg {
    uint64_t destId;     // The node to which the data should be piggybacked
    int payloadSz;  // the size the payload would have in the "real" packet
                    // (in Byte)
    bool inTransit; // whether the data is currently being piggybacked
    // NOTE: payload is set as context Pointer!
}