//
//  Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
//  Copyright (C) 2023  Institute of Communication Networks (ComNets),
//                      Hamburg University of Technology (TUHH)
//            (C) 2023  Yevhenii Shudrenko
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

#ifndef APPLICATIONS_UDPAPP_UDPBURSTAPP_H_
#define APPLICATIONS_UDPAPP_UDPBURSTAPP_H_

#include <vector>

#include "inet/common/INETDefs.h"

#include "inet/applications/udpapp/UdpBasicApp.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

using namespace inet;

namespace tsch{

class UdpBurstApp: public inet::UdpBasicApp {
    public:
        UdpBurstApp();
        virtual ~UdpBurstApp();

    protected:
        enum SelfMsgKinds { START = 1, SEND, STOP, SEND_BURST };
        virtual void initialize(int stage) override;
        virtual void finish() override;
        virtual void processSend() override;
        virtual void handleMessageWhenUp(cMessage *msg) override;

        virtual void processSendBurst();

    private:
        int burstSize;
        int numBursts;
        int processSents;
        double pBurst;
        double burstPeriod;
        cMessage* burstTriggerMsg;

        simsignal_t burstArrivedSignal;
};
}

#endif /* APPLICATIONS_UDPAPP_UDPBURSTAPP_H_ */
