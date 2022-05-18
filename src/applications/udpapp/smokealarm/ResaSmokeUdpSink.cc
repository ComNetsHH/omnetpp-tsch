//
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

#include "ResaSmokeUdpSink.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"
#include <regex>
//#include <stdio.h>
//#include <string.h>


using namespace inet;

namespace tsch {

Define_Module(ResaSmokeUdpSink);

ResaSmokeUdpSink::ResaSmokeUdpSink() {
    // TODO Auto-generated constructor stub

}

ResaSmokeUdpSink::~ResaSmokeUdpSink() {
    // TODO Auto-generated destructor stub
}

void ResaSmokeUdpSink::processPacket(Packet *pk)
{
    EV_INFO << "Received packet: " << UdpSocket::getReceivedPacketInfo(pk)
            << "\ntotal: " << numReceived << ", num to receive: " << par("numPktsToReceive").intValue() << endl;

    auto sender = UdpSocket::getPacketSrcAddress(pk);

    // this method of redundant retransmissions only works if a single packet is expected,
    // since otherwise it's (almost) impossible to differentiate between copies of the same packet and
    // a new one. Copies of the same packet have different sequence numbers and look like completely
    // different packets for the application.
    if (numReceivedPktsPerNeighbor.find(sender) == numReceivedPktsPerNeighbor.end())
        numReceivedPktsPerNeighbor[sender] = 0;
    else if (numReceivedPktsPerNeighbor[sender] == 1)
        return;

    emit(packetReceivedSignal, pk);
    delete pk;

    numReceived++;
    numReceivedPktsPerNeighbor[sender]++;
}


} // namespace tsch


