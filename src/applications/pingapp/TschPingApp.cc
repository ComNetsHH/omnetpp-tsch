//
//  Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
//  Copyright (C) 2019  Institute of Communication Networks (ComNets),
//                      Hamburg University of Technology (TUHH)
//            (C) 2019  Leo Krueger, Louis Yin
//            (C) 2005  Andras Varga
//            (C) 2001, 2003, 2004 Johnny Lai, Monash University, Melbourne, Australia
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "TschPingApp.h"
#include "../../common/VirtualLinkTag_m.h"

#include "inet/applications/pingapp/PingApp.h"
#include "inet/applications/pingapp/PingApp_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Protocol.h"
#include "inet/common/ProtocolGroup.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/common/packet/chunk/ByteCountChunk.h"
#include "inet/networklayer/common/EchoPacket_m.h"
#include "inet/networklayer/common/HopLimitTag_m.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/networklayer/common/IpProtocolId_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/IL3AddressType.h"
#include "inet/networklayer/contract/L3Socket.h"
#include "inet/networklayer/contract/ipv4/Ipv4Socket.h"
#include "inet/networklayer/contract/ipv6/Ipv6Socket.h"

#ifdef WITH_IPv4
#include "inet/networklayer/ipv4/Icmp.h"
#include "inet/networklayer/ipv4/IcmpHeader.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#endif // ifdef WITH_IPv4

#ifdef WITH_IPv6
#include "inet/networklayer/icmpv6/Icmpv6.h"
#include "inet/networklayer/icmpv6/Icmpv6Header_m.h"
#include "inet/networklayer/ipv6/Ipv6InterfaceData.h"
#endif // ifdef WITH_IPv6




using namespace inet;

using std::cout;

Define_Module(tsch::TschPingApp);


enum PingSelfKinds {
    PING_FIRST_ADDR = 1001,
    PING_CHANGE_ADDR,
    PING_SEND
};

namespace tsch {
TschPingApp::TschPingApp() {
}

TschPingApp::~TschPingApp() {
//    cancelAndDelete(timer);
//    socketMap.deleteSockets();
}


void TschPingApp::initialize(int stage) {
    PingApp::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        virtualLinkID = par("VirtualLinkID");
    }
}


void TschPingApp::sendPingRequest()
{
    char name[32];
    sprintf(name, "ping%ld", sendSeqNo);

    ASSERT(pid != -1);

    Packet *outPacket = new Packet(name);
    auto payload = makeShared<ByteCountChunk>(B(packetSize));

    switch (destAddr.getType()) {
        case L3Address::IPv4: {
#ifdef WITH_IPv4
            const auto& request = makeShared<IcmpEchoRequest>();
            request->setIdentifier(pid);
            request->setSeqNumber(sendSeqNo);
            outPacket->insertAtBack(payload);
            Icmp::insertCrc(crcMode, request, outPacket);
            outPacket->insertAtFront(request);
            outPacket->addTag<PacketProtocolTag>()->setProtocol(&Protocol::icmpv4);
            break;
#else
            throw cRuntimeError("INET compiled without Ipv4");
#endif
        }
        case L3Address::IPv6: {
#ifdef WITH_IPv6
            const auto& request = makeShared<Icmpv6EchoRequestMsg>();
            request->setIdentifier(pid);
            request->setSeqNumber(sendSeqNo);
            outPacket->insertAtBack(payload);
            Icmpv6::insertCrc(crcMode, request, outPacket);
            outPacket->insertAtFront(request);
            outPacket->addTag<PacketProtocolTag>()->setProtocol(&Protocol::icmpv6);
            break;
#else
            throw cRuntimeError("INET compiled without Ipv6");
#endif
        }
        case L3Address::MODULEID:
        case L3Address::MODULEPATH: {
#ifdef WITH_NEXTHOP
            const auto& request = makeShared<EchoPacket>();
            request->setChunkLength(B(8));
            request->setType(ECHO_PROTOCOL_REQUEST);
            request->setIdentifier(pid);
            request->setSeqNumber(sendSeqNo);
            outPacket->insertAtBack(payload);
            // insertCrc(crcMode, request, outPacket);
            outPacket->insertAtFront(request);
            outPacket->addTag<PacketProtocolTag>()->setProtocol(&Protocol::echo);
            break;
#else
            throw cRuntimeError("INET compiled without Next Hop Forwarding");
#endif
        }
        default:
            throw cRuntimeError("Unaccepted destination address type: %d (address: %s)", (int)destAddr.getType(), destAddr.str().c_str());
    }

    auto addressReq = outPacket->addTag<L3AddressReq>();
    addressReq->setSrcAddress(srcAddr);
    addressReq->setDestAddress(destAddr);
    /// ************* //////////
    /// Make changes here //////
    auto tag = outPacket->addTagIfAbsent<VirtualLinkTagReq>();
    tag->setVirtualLinkID(virtualLinkID);
    if (hopLimit != -1)
        outPacket->addTag<HopLimitReq>()->setHopLimit(hopLimit);
    EV_INFO << "Sending ping request #" << sendSeqNo << " to lower layer.\n";
    currentSocket->send(outPacket);

    // store the sending time in a circular buffer so we can compute RTT when the packet returns
    sendTimeHistory[sendSeqNo % PING_HISTORY_SIZE] = simTime();
    pongReceived[sendSeqNo % PING_HISTORY_SIZE] = false;
    emit(pingTxSeqSignal, sendSeqNo);

    sendSeqNo++;
    sentCount++;
}
}
