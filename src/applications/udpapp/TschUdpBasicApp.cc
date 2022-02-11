//
//  Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
//  Copyright (C) 2019  Institute of Communication Networks (ComNets),
//                      Hamburg University of Technology (TUHH)
//            (C) 2019  Louis Yin
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

#include "TschUdpBasicApp.h"
#include "../../common/VirtualLinkTag_m.h"
#include <iostream>
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/applications/udpapp/UdpBasicApp.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/FragmentationTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"



using namespace inet;

Define_Module(tsch::TschUdpBasicApp);

namespace tsch {

TschUdpBasicApp::TschUdpBasicApp() {
    // TODO Auto-generated constructor stub

}

TschUdpBasicApp::~TschUdpBasicApp() {
    // TODO Auto-generated destructor stub
}

void TschUdpBasicApp::initialize(int stage){
    ApplicationBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
            numSent = 0;
            numReceived = 0;
            WATCH(numSent);
            WATCH(numReceived);

            localPort = par("localPort");
            destPort = par("destPort");
            startTime = par("startTime");
            stopTime = par("stopTime");
            packetName = par("packetName");
            dontFragment = par("dontFragment");
            virtualLinkId = par("virtualLinkId");
            if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
                throw cRuntimeError("Invalid startTime/stopTime parameters");
            selfMsg = new cMessage("sendTimer");
    }
}

void TschUdpBasicApp::sendPacket(){
    std::ostringstream str;
    str << packetName << "-" << numSent;
    Packet *packet = new Packet(str.str().c_str());
    if(dontFragment)
        packet->addTagIfAbsent<FragmentationReq>()->setDontFragment(true);
    auto tag = packet->addTagIfAbsent<VirtualLinkTagReq>();
    tag->setVirtualLinkID(virtualLinkId);
    const auto& payload = makeShared<ApplicationPacket>();
    payload->setChunkLength(B(par("messageLength")));
    payload->setSequenceNumber(numSent);
    payload->addTag<CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(payload);
    L3Address destAddr = chooseDestAddr();
    emit(packetSentSignal, packet);
    socket.sendTo(packet, destAddr, destPort);
    numSent++;
}
}


