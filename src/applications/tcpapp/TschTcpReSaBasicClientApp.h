//
//  Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
//  Copyright (C) 2022  Institute of Communication Networks (ComNets),
//                      Hamburg University of Technology (TUHH)
//            (C) 2022  Gökay Apusoglu
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

#ifndef APPLICATIONS_TCPAPP_TSCHTCPRESABASICCLIENTAPP_H_
#define APPLICATIONS_TCPAPP_TSCHTCPRESABASICCLIENTAPP_H_

#include <vector>

#include "inet/common/INETDefs.h"

#include "inet/applications/tcpapp/TcpBasicClientApp.h"
#include "inet/common/lifecycle/ModuleOperations.h"

using namespace inet;

namespace tsch{

class TschTcpReSaBasicClientApp: public inet::TcpBasicClientApp {
public:
    TschTcpReSaBasicClientApp();
    virtual ~TschTcpReSaBasicClientApp();

protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

    double activeUserRatio; // Ratio of the active application user; some users may not use their application at all based on this ratio.
    cMessage *activationCheck = nullptr;
};
}

#endif /* APPLICATIONS_TCPAPP_TSCHTCPRESABASICCLIENTAPP_H_ */
