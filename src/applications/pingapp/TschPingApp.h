//
//  Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
//  Copyright (C) 2021  Institute of Communication Networks (ComNets),
//                      Hamburg University of Technology (TUHH)
//            (C) 2019  Leo Krueger, Louis Yin
//            (C) 2005  Andras Varga
//            (C) 2001, 2003, 2004 Johnny Lai, Monash University, Melbourne, Australia
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


#ifndef LINKLAYER_IEEE802154E_TSCHPINGAPP_H_
#define LINKLAYER_IEEE802154E_TSCHPINGAPP_H_

#include "inet/applications/pingapp/PingApp.h"

using namespace inet;

namespace tsch {

class TschPingApp: public inet::PingApp {
public:
    TschPingApp();
    virtual ~TschPingApp();
protected:
    virtual void sendPingRequest() override;
    virtual void initialize(int stage) override;
private:
    int virtualLinkID;
};
}

#endif /* LINKLAYER_IEEE802154E_TSCHPINGAPP_H_ */
