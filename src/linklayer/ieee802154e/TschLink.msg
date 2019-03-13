//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//


cplusplus {{
#include "TschLink.h"
#include "inet/linklayer/common/MacAddress.h"
typedef inet::MacAddress MacAddress;
}}

namespace tsch;
class noncobject MacAddress;

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
    
    MacAddress addr @byValue;
}
