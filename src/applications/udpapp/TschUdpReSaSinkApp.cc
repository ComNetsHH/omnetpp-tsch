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

#include "TschUdpReSaSinkApp.h"

#include "inet/applications/udpapp/UdpSink.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/TagBase_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

using namespace inet;

Define_Module(tsch::TschUdpReSaSinkApp);

namespace tsch {

TschUdpReSaSinkApp::TschUdpReSaSinkApp() {
    // TODO Auto-generated constructor stub

}

TschUdpReSaSinkApp::~TschUdpReSaSinkApp() {
    // TODO Auto-generated destructor stub
}

void TschUdpReSaSinkApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        numReceived = 0;
        WATCH(numReceived);

        localPort = par("localPort");
        startTime = par("startTime");
        stopTime = par("stopTime");
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        selfMsg = new cMessage("UDPSinkTimer");
        moduleIndex_int = 0;

        WATCH_MAP(hazardPkDelay);
        WATCH(hazardPkMeanDelay);
    }
}

void TschUdpReSaSinkApp::socketDataArrived(UdpSocket *socket, Packet *pk)
{

    const char* pkName = pk->getName();

    if (strncmp(pkName, "HAZARD", 6) == 0) {

        // For tracing HAZARD packets easily during the debug process, sequence number is set to the packet-owner smoke sensor's index number.
        std::string sender = pkName;
        std::string moduleIndex;
        std::size_t begin = sender.find("[");
        std::size_t end = sender.find("]",begin);
        moduleIndex = sender.substr(begin+1,end-begin-1);
        moduleIndex_int= stoi(moduleIndex);
        const double delay = (pk->getSendingTime()).dbl() - (pk->getCreationTime()).dbl();
        hazardPkDelay.insert({delay,moduleIndex_int});
        hazardPkMeanDelay = (hazardPkMeanDelay*(hazardPkDelay.size()-1) + delay)/hazardPkDelay.size();

        recordScalar("hazardPkMeanDownlinkDelay", hazardPkMeanDelay);

        // process incoming packet
        processPacket(pk);
    }

}
}


