//
//  Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
//  Copyright (C) 2021  Institute of Communication Networks (ComNets),
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

#ifndef APPLICATIONS_UDPAPP_TSCHUDPRESASINKAPP_H_
#define APPLICATIONS_UDPAPP_TSCHUDPRESASINKAPP_H_

#include <vector>

#include "inet/common/INETDefs.h"

#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

#include "inet/applications/udpapp/UdpSink.h"
#include "inet/common/INETDefs.h"
#include <fstream>
#include <iostream>
#include <list>
#include <cstdio>
#include <cstring>
#include <regex>
#include <omnetpp.h>
#include <algorithm>

using namespace omnetpp;
using namespace inet;

namespace tsch{

class TschUdpReSaSinkApp: public inet::UdpSink {
public:
    TschUdpReSaSinkApp();
    ~TschUdpReSaSinkApp();

protected:
    int moduleIndex_int;

    std::map<double, int> hazardPkDelay;
    double hazardPkMeanDelay;

    friend std::ostream& operator<<(std::ostream& os, std::map<double, int> hazardPkDelay)
        {
            for (auto const delay: hazardPkDelay){
                os << "Delay: " << std::get<0>(delay) << "s     Source:sos[" << std::get<0>(delay) << "]\n" << std::endl;
            }
            return os;
        }

    virtual void initialize(int stage) override;
    virtual void socketDataArrived(UdpSocket *socket, Packet *pk) override;

};
}

#endif /* APPLICATIONS_UDPAPP_TSCHUDPRESASINKAPP_H_ */
