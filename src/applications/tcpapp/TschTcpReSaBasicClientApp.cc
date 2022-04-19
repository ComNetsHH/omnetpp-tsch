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

#include "TschTcpReSaBasicClientApp.h"

#include "inet/applications/tcpapp/TcpBasicClientApp.h"
#include "inet/applications/tcpapp/GenericAppMsg_m.h"
#include "inet/common/lifecycle/LifecycleController.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/common/StringFormat.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TimeTag_m.h"
#include <algorithm>
#include <array>


using namespace inet;

Define_Module(tsch::TschTcpReSaBasicClientApp);

namespace tsch {

TschTcpReSaBasicClientApp::TschTcpReSaBasicClientApp() {
    // TODO Auto-generated constructor stub

}

TschTcpReSaBasicClientApp::~TschTcpReSaBasicClientApp() {
    // TODO Auto-generated destructor stub
}

void TschTcpReSaBasicClientApp::initialize(int stage){

    TcpBasicClientApp::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        activeUserRatio = par("activeUserRatio");
        activationCheck = new cMessage("activationCheck");
        scheduleAt(simTime()+SimTime(1,SIMTIME_PS),activationCheck);
        }
}


void TschTcpReSaBasicClientApp::handleMessage(cMessage *msg)
{
    if (msg == activationCheck){
        delete msg;
        int numUsers = this->getSystemModule()->getAncestorPar("numUsers");
        if (activeUserRatio != 1 && activeUserRatio != 0){
            double numInactive = numUsers - ceil(numUsers*activeUserRatio);
            if (numInactive != 0){
                std::vector<double> inactiveUsers(numInactive);
                double n = 0;
                double step = (numUsers/numInactive);
                std::generate(inactiveUsers.begin(), inactiveUsers.end(), [&n,&step]{ return n+=step; });

                for(auto & el : inactiveUsers){el=round(el);};

                bool found = (std::find(inactiveUsers.begin(), inactiveUsers.end(), this->getParentModule()->getIndex()) != inactiveUsers.end());

                // If found in the list, disable
                auto parentModule = this->getParentModule();
                if (found){
                        parentModule->deleteModule();
                }
            }

        }
        else if(activeUserRatio == 0){
            auto parentModule = this->getParentModule();
            parentModule->deleteModule();
        }
    }
    else{
        OperationalBase::handleMessage(msg);
        }
}

}
