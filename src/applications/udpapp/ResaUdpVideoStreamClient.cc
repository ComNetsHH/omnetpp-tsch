//
// Copyright (C) 2005 Andras Varga
//
// Based on the video streaming app of the similar name by Johnny Lai.
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

#include "ResaUdpVideoStreamClient.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/chunk/ByteCountChunk.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

using namespace inet;

namespace tsch {

Define_Module(ResaUdpVideoStreamClient);

void ResaUdpVideoStreamClient::initialize(int stage) {
    UdpVideoStreamClient::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        numReceived = 0;
        WATCH(numReceived);
    }
}


void ResaUdpVideoStreamClient::receiveStream(Packet *pk)
{
    EV_INFO << "Video stream packet: " << UdpSocket::getReceivedPacketInfo(pk) << endl;
    emit(packetReceivedSignal, pk);
    numReceived++;
    delete pk;
}

} // namespace inet

