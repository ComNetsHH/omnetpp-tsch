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

#include "Ipv6NeighbourDiscoveryWireless.h"

namespace tsch {

Define_Module(Ipv6NeighbourDiscoveryWireless);

void Ipv6NeighbourDiscoveryWireless::initialize(int stage)
{
    Ipv6NeighbourDiscovery::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        naSolicitedPacketSent = registerSignal("naSolicitedPacketSent");
        naUnsolicitedPacketSent = registerSignal("naUnsolicitedPacketSent");
        nsPacketSent = registerSignal("nsPacketSent");
    }
}

void Ipv6NeighbourDiscoveryWireless::createAndSendNsPacket(const Ipv6Address& nsTargetAddr, const Ipv6Address& dgDestAddr,
                const Ipv6Address& dgSrcAddr, InterfaceEntry *ie)
{
    Ipv6NeighbourDiscovery::createAndSendNsPacket(nsTargetAddr, dgDestAddr, dgSrcAddr, ie);
    emit(nsPacketSent, 1);
}

void Ipv6NeighbourDiscoveryWireless::sendUnsolicitedNa(InterfaceEntry *ie)
{
    Ipv6NeighbourDiscovery::sendUnsolicitedNa(ie);
    emit(naUnsolicitedPacketSent, 1);
}

void Ipv6NeighbourDiscoveryWireless::sendSolicitedNa(Packet *packet,
            const Ipv6NeighbourSolicitation *ns, InterfaceEntry *ie)
{
    Ipv6NeighbourDiscovery::sendSolicitedNa(packet, ns, ie);
    emit(naSolicitedPacketSent, 1);
}


} // namespace tsch

