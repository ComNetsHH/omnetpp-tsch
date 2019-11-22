// 
// Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
// Copyright (C) 2019  Institute of Communication Networks (ComNets), 
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


cplusplus {{
#include "TschLink.h"
#include "inet/linklayer/common/MacAddress.h"
}}

class inet::MacAddress { @existingClass; }

namespace tsch;

class TschLink extends cNamedObject
{
    @existingClass;
    @descriptor(readonly);
    bool optionTx @getter(isTx) @setter(setTx);
    bool optionRx @getter(isRx) @setter(setRx);
    bool optionShared @getter(isShared) @setter(setShared);
    bool optionTimekeeping @getter(isTimekeeping) @setter(setTimekeeping);    // TODO unused
    
    bool typeNormal @getter(isNormal) @setter(setNormal);
    bool typeAdvertising @getter(isAdv) @setter(setAdv);      // TODO unused
    bool typeAdvertisingOnly @getter(isAdvOnly) @setter(setAdvOnly);  // TODO unused
    
    int channelOffset;
    int slotOffset;
    
    inet::MacAddress addr @byValue;
}
