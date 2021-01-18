//
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
// Control messages sent by the 6top sublayer to the mac layer
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
#include "WaicCellComponents.h"
#include "TschSF.h"

#include <stdint.h>
#include <vector>

typedef macLinkOption_t macLinkOption;
}}

class cellListVector { @existingClass;}
enum macLinkOption_t;
//class macLinkOption { @existingClass; }


message tsch6topCtrlMsg {
    simtime_t timeout;           // The absolute time at which this message
                                 // becomes invalid
    uint64_t destId;                  // The neighbor affected by the pattern update
    cellListVector newCells;     // The cells to be added
    cellListVector deleteCells;  // The cells to be added
    int cellOptions @enum(macLinkOption_t); // The linkOptions that apply to all links
                                 // in cellList
}