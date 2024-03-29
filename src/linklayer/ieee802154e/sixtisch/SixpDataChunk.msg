//
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2021  Institute of Communication Networks (ComNets),
//                     Hamburg University of Technology (TUHH)
//           (C) 2017  Lotte Steenbrink
//           (C) 2016  Andras Varga
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

cplusplus {{
#include "WaicCellComponents.h"
#include "Tsch6tischComponents.h"
#include <vector>
}}

class cellListVector { @existingClass; @descriptor(false); }

namespace tsch::sixtisch;

class SixpData extends inet::FieldsChunk {
    int cellOptions;
    int numCells;
    cellListVector cellList; // the proposed/accepted cells. In relocation
                             // requests: the candiateCellList
    cellListVector relocationCellList;  // only used in relocation requests
    simtime_t timeout; // absolute timeout for this transaction.
                    // (this isn't 100% according to the standard as it should
                    // be set in the metadata field but let's not
                    // overcomplicate this.)
}