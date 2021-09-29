//
//  Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
//  Copyright (C) 2019  Institute of Communication Networks (ComNets),
//                      Hamburg University of Technology (TUHH)
//            (C) 2021  Gökay Apusoglu
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

#ifndef APPLICATIONS_UDPAPP_TSCHUDPRESAECHOAPP_H_
#define APPLICATIONS_UDPAPP_TSCHUDPRESAECHOAPP_H_

#include "inet/applications/udpapp/UdpEchoApp.h"
#include "inet/common/INETDefs.h"

#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include <fstream>
#include <iostream>
#include <list>
#include <cstdio>
#include <cstring>
#include <regex>

using namespace inet;

namespace tsch{

class TschUdpReSaEchoApp: public inet::UdpEchoApp {
public:
    TschUdpReSaEchoApp();
    virtual ~TschUdpReSaEchoApp();
protected:
    L3Address destAddr;
    L3Address amAddr;
    std::vector<L3Address> amAddrList;
    std::vector<std::string> amAddrListStr;

    int destPort;
    int amPort;

    int rcvdPkNo;

    virtual void initialize(int stage) override;
    virtual void socketDataArrived(UdpSocket *socket, Packet *packet) override;
    virtual void forwardPacket(UdpSocket *socket, Packet *packet, L3Address destAddr, int destPort, const char* pkName);
};
}

#endif /* APPLICATIONS_UDPAPP_TSCHUDPRESAECHOAPP_H_ */
