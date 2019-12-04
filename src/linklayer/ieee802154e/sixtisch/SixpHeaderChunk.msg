//
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
// Message type used by the minimal 6p implementation
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

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

namespace tsch::sixtisch;

class SixpHeader extends inet::FieldsChunk {
    int version = 0;
    int type = -1;
    int code = -1;
    int sfid = -1;
    int seqNum = -1;
}