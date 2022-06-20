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

#include "WaicUdpSink.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

using namespace inet;

namespace tsch {

Define_Module(WaicUdpSink);

WaicUdpSink::WaicUdpSink() {
    // TODO Auto-generated constructor stub

}

WaicUdpSink::~WaicUdpSink() {
    // TODO Auto-generated destructor stub
}

void WaicUdpSink::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        numReceived = 0;
        jitterRecorder = registerSignal("jitterRecorder");
        WATCH(numReceived);

        localPort = par("localPort");
        startTime = par("startTime");
        stopTime = par("stopTime");
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        selfMsg = new cMessage("UDPSinkTimer");
    }
}


void WaicUdpSink::finish()
{
    UdpSink::finish();
    EV_INFO << getFullPath() << ": received " << numReceived << " packets\n";
    simtime_t meanJitter = 0;
    for (auto jt: jitterMap) {
        auto jitter = computeJitter(jt.second);
        meanJitter += jitter;
        emit(jitterRecorder, jitter);
    }
}

simtime_t WaicUdpSink::getPacketDelay(Packet *pk) {
    auto creationTimeTag = pk->peekData()->findTag<CreationTimeTag>();
    return simTime() - creationTimeTag->getCreationTime();
}

L3Address WaicUdpSink::getPacketSrcAddress(Packet *pk) {
    auto l3Addresses = pk->getTag<L3AddressInd>();
    return l3Addresses->getSrcAddress();
}

void WaicUdpSink::processPacket(Packet *pk)
{
    EV_INFO << "Received packet: " << UdpSocket::getReceivedPacketInfo(pk) << endl;

    auto srcAddr = getPacketSrcAddress(pk);
    auto pkDelay = getPacketDelay(pk);

    auto jitterEntry = jitterMap.find(srcAddr);

    if (jitterEntry == jitterMap.end())
        jitterMap.insert(std::pair<L3Address, std::vector<simtime_t>> (srcAddr, {pkDelay}));
    else
        jitterEntry->second.push_back(pkDelay);

    emit(packetReceivedSignal, pk);
    delete pk;

    numReceived++;
}

simtime_t WaicUdpSink::computeJitter(std::vector<simtime_t> delays) {
    double diff = 0;
    for (auto i = 0; i < delays.size()-1; i++)
        diff += abs(delays[i+1].dbl() - delays[i].dbl());

    return SimTime(diff / (double) delays.size());
}

} // namespace tsch


