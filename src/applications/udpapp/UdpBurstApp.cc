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

#include "UdpBurstApp.h"
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

Define_Module(tsch::UdpBurstApp);

namespace tsch {

UdpBurstApp::UdpBurstApp() {
    // TODO Auto-generated constructor stub

}

UdpBurstApp::~UdpBurstApp() {
    // TODO Auto-generated destructor stub
}

void UdpBurstApp::initialize(int stage){
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
            burstSize = par("burstSize").intValue();
            pBurst = par("pBurst").doubleValue();
            burstPeriod = par("burstPeriod").doubleValue();
            numBursts = 0;
            processSents = 0;

            if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
                throw cRuntimeError("Invalid startTime/stopTime parameters");
            selfMsg = new cMessage("sendTimer");

            burstTriggerMsg = new cMessage("triggerBurst", SEND_BURST);
            burstArrivedSignal = registerSignal("burstArrived");
    }
}


void UdpBurstApp::finish()
{
    auto pBurstEmpirical = (double) numBursts/processSents;
    std::cout << "p_b (empirical) = " << pBurstEmpirical << ", configured = " << pBurst << endl;
    recordScalar("pbEmpirical", pBurstEmpirical);
    UdpBasicApp::finish();
}


void UdpBurstApp::processSend()
{
    simtime_t d = simTime() + par("sendInterval");

    auto sendBurstOffset = uniform(0, 0.99, 2) * par("sendInterval").doubleValue();
    scheduleAt(simTime() + sendBurstOffset, burstTriggerMsg);

    EV << "Scheduled burst trigger with offset " << sendBurstOffset << " s at " << simTime() + sendBurstOffset << " s" << endl;

    if (stopTime < SIMTIME_ZERO || d < stopTime) {
        selfMsg->setKind(SEND);
        scheduleAt(d, selfMsg);
    }
    else {
        selfMsg->setKind(STOP);
        scheduleAt(stopTime, selfMsg);
    }
}

void UdpBurstApp::processSendBurst()
{
    processSents++;
    bool toSendBurst = uniform(0, 1) < pBurst;
    EV << "Random draw - " << toSendBurst << ", burst probability - " << pBurst << endl;

    if (toSendBurst)
    {
        numBursts++;
        for (auto i = 0; i < burstSize; i++)
            sendPacket();
    }
}

void UdpBurstApp::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {

        switch (msg->getKind()) {
            case START:
                processStart();
                break;

            case SEND:
                processSend();
                break;

            case SEND_BURST:
                processSendBurst();
                break;

            case STOP:
                processStop();
                break;

            default:
                throw cRuntimeError("Invalid kind %d in self message", (int)selfMsg->getKind());
        }
    }
    else
        socket.processMessage(msg);
}

}


