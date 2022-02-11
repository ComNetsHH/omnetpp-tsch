//
//  Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
//  Copyright (C) 2019  Institute of Communication Networks (ComNets),
//                      Hamburg University of Technology (TUHH)
//            (C) 2021  G�kay Apusoglu
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

#include "TschUdpReSaEchoApp.h"

#include "../../common/VirtualLinkTag_m.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/applications/udpapp/UdpEchoApp.h"
#include "inet/applications/udpapp/UdpBasicApp.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/TagBase_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/FragmentationTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

using namespace inet;

Define_Module(tsch::TschUdpReSaEchoApp);

namespace tsch{

TschUdpReSaEchoApp::TschUdpReSaEchoApp() {
    // TODO Auto-generated constructor stub

}

TschUdpReSaEchoApp::~TschUdpReSaEchoApp() {
    // TODO Auto-generated destructor stub
}

void TschUdpReSaEchoApp::initialize(int stage){
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        // init statistics
        numEchoed = 0;
        moduleIndex_int = 0;
        WATCH(numEchoed);
        WATCH_MAP(hazardPkDelay);
        WATCH(hazardPkMeanDelay);

    }
    else if(stage == INITSTAGE_LAST){
        amPort = par("amPort");
    }

}
void TschUdpReSaEchoApp::socketDataArrived(UdpSocket *socket, Packet *pk)
{
    // determine its source address/port
    EV_INFO << "Received packet: " << UdpSocket::getReceivedPacketInfo(pk) << endl;
    auto l3Addresses = pk->getTag<L3AddressInd>();
    auto ports = pk->getTag<L4PortInd>();
    //L3Address srcAddr = l3Addresses->getSrcAddress();
    //int srcPort = ports->getSrcPort();

    pk->clearTags();
    pk->trim();
    const char* pkName = pk->getName();


    // send back
    if (strncmp(pkName, "HAZARD", 6) == 0) {

        // For tracing HAZARD packets easily during the debug process, sequence number is set to the packet-owner smoke sensor's index number.
        std::string sender = pkName;
        std::string moduleIndex;
        std::size_t begin = sender.find("[");
        std::size_t end = sender.find("]",begin);
        moduleIndex = sender.substr(begin+1,end-begin-1);
        moduleIndex_int = stoi(moduleIndex);
        const double delay = (pk->getArrivalTime()).dbl() - (pk->getCreationTime()).dbl();
        hazardPkDelay.insert({delay,moduleIndex_int});
        hazardPkMeanDelay = (hazardPkMeanDelay*(hazardPkDelay.size()-1) + delay)/hazardPkDelay.size();
        recordScalar("hazardPkMeanUplinkDelay", hazardPkMeanDelay);

        const char *addrs = par("amAddrList");
        cStringTokenizer tokenizer(addrs);
        const char *token;
        while ((token = tokenizer.nextToken()) != nullptr) {
            amAddrListStr.push_back(token);

            EV_INFO << "HAZARD PACKET IS SENT TO " << token <<endl;

            L3AddressResolver().tryResolve(token, destAddr);
            if (destAddr.isUnspecified())
               EV_ERROR << "cannot resolve destination address: " << token << endl;
            destPort = amPort;
            forwardPacket(socket, pk, destAddr, destPort, pkName);
       }

    }
    delete pk;
}

void TschUdpReSaEchoApp::forwardPacket(UdpSocket *socket, Packet *packet, L3Address destAddr, int destPort, const char* pkName){
    std::ostringstream str;
    str << "Port:" << destPort;
    Packet *echopacket = new Packet(str.str().c_str());
    echopacket->setName(pkName);

    const auto& payload = makeShared<ApplicationPacket>();
    payload->setChunkLength(B(packet->getByteLength()));
    payload->setSequenceNumber(moduleIndex_int);
    payload->addTag<CreationTimeTag>()->setCreationTime(packet->getCreationTime());
    echopacket->addTagIfAbsent<VirtualLinkTagReq>()->setVirtualLinkID(par("virtualLinkId").intValue());
    echopacket->insertAtBack(payload);
    emit(packetSentSignal, echopacket);

    socket->sendTo(echopacket,destAddr,destPort);
    numEchoed++;
}


}

