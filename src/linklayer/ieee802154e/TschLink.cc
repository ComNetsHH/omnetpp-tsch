//
// Copyright (C) 2004-2006 Andras Varga
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
//

#include <sstream>
#include <stdio.h>

#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/networklayer/ipv4/IIpv4RoutingTable.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "TschSlotframe.h"
#include "inet/networklayer/ipv4/Ipv4RoutingTable.h"

class TschSlotframe;

namespace tsch {

Register_Class(TschLink);

TschLink::~TschLink()
{

}

std::string TschLink::str() const
{
    std::stringstream out;
    out << "[ " << slotOffset << ", ";
    out << channelOffset << " ] ";
    if (type_normal)
        out << "NORM ";
    if (type_advertising)
        out << "ADV ";
    if (type_advertisingOnly)
        out << "ADVonly ";
    if (option_tx)
        out << "TX ";
    if (option_rx)
        out << "RX ";
    if (option_shared)
        out << "SHARED ";
    if (option_timekeeping)
        out << "TIME ";
    out << addr.str();

    return out.str();
}

std::string TschLink::detailedInfo() const
{
    return std::string();
}

bool TschLink::equals(const TschLink& link) const
{
    return sf == link.sf && slotOffset == link.slotOffset; // TODO which fields to compare?
}

void TschLink::changed(int fieldCode)
{
    if (sf)
        sf->linkChanged(this, fieldCode);
}

} // namespace tsch

