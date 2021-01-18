//
//  Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
//  Copyright (C) 2021  Institute of Communication Networks (ComNets),
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

#include "TschUdpEchoApp.h"
#include "../../common/VirtualLinkTag_m.h"
#include "inet/applications/udpapp/UdpEchoApp.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/TagBase_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"


using namespace inet;

Define_Module(tsch::TschUdpEchoApp);

namespace tsch{

TschUdpEchoApp::TschUdpEchoApp() {
    // TODO Auto-generated constructor stub

}

TschUdpEchoApp::~TschUdpEchoApp() {
    // TODO Auto-generated destructor stub
}

void TschUdpEchoApp::socketDataArrived(UdpSocket *socket, Packet *pk)
{
    // determine its source address/port
    L3Address remoteAddress = pk->getTag<L3AddressInd>()->getSrcAddress();
    int srcPort = pk->getTag<L4PortInd>()->getSrcPort();
    int virtualLinkID;
    auto virtualLinkTagInd = pk->findTag<VirtualLinkTagInd>();
    if (virtualLinkTagInd != nullptr){
        virtualLinkID = virtualLinkTagInd->getVirtualLinkID();
    }
    pk->clearTags();
    pk->trim();

    // statistics
    numEchoed++;
    emit(packetSentSignal, pk);
    //
    // Instead of changing also the Udp socket, the packet is already modified here
    if (virtualLinkTagInd != nullptr){
        pk->addTagIfAbsent<VirtualLinkTagReq>()->setVirtualLinkID(virtualLinkID);
    }

    // send back

    socket->sendTo(pk, remoteAddress, srcPort);
}
}

