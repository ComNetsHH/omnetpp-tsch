// 
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2019  Institute of Communication Networks (ComNets), 
//                     Hamburg University of Technology (TUHH)
//           (C) 2019  Leo Krueger, Louis Yin
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
#include "TschVirtualLink.h"
}}

import TschLink;

namespace tsch;

// Defines a virtual link.
// Non-standard tsch behaviour, helpful to mimic other/similar MACs
class TschVirtualLink extends TschLink
{
    @existingClass;
    @descriptor(readonly);

	// define a virtual link id, usable to distinguish and
	// separate multiple applications using the same link/node
    int virtualLinkId @getter(getVirtualLink) @setter(setVirtualLink);

	// set link as usable for retransmissions only
    bool optionRtx @getter(isRtx) @setter(setRtx);
}

