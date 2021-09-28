/**
 * Copyright (C) 2005 Andras Varga
 * Copyright (C) 2005 Wei Yang, Ng
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NETWORKLAYER_IPV6NEIGHBOURDISCOVERYWIRELESS_H
#define NETWORKLAYER_IPV6NEIGHBOURDISCOVERYWIRELESS_H

#include "inet/networklayer/icmpv6/Ipv6NeighbourDiscovery.h"

using namespace inet;

namespace tsch {

class Ipv6NeighbourDiscoveryWireless : public Ipv6NeighbourDiscovery
{
protected:
    virtual void initialize(int stage) override;
    virtual void createAndSendNsPacket(const Ipv6Address& nsTargetAddr, const Ipv6Address& dgDestAddr,
                const Ipv6Address& dgSrcAddr, InterfaceEntry *ie) override;

    virtual void sendSolicitedNa(Packet *packet,
            const Ipv6NeighbourSolicitation *ns, InterfaceEntry *ie) override;

    virtual void sendUnsolicitedNa(InterfaceEntry *ie) override;

    simsignal_t naSolicitedPacketSent;
    simsignal_t naUnsolicitedPacketSent;
    simsignal_t nsPacketSent;
};

}
#endif    //NETWORKLAYER_IPV6NEIGHBOURDISCOVERYWIRELESS_H

