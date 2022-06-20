//
// Copyright (C) 2004 Andras Varga
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

#ifndef APPLICATIONS_UDPAPP_WAICUDPSINK_H_
#define APPLICATIONS_UDPAPP_WAICUDPSINK_H_

#include "inet/common/INETDefs.h"
#include "inet/applications/udpapp/UdpSink.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/common/TimeTag_m.h"

using namespace inet;

namespace tsch {

class WaicUdpSink : public UdpSink
{
  protected:
    std::map<L3Address, std::vector<simtime_t>> jitterMap;
    simsignal_t jitterRecorder;

  public:
    WaicUdpSink();
    virtual ~WaicUdpSink();

    static simtime_t computeJitter(std::vector<simtime_t> delays);
    L3Address getPacketSrcAddress(Packet *pk);
    simtime_t getPacketDelay(Packet *pk);

  protected:
    virtual void processPacket(Packet *msg) override;

  protected:
    virtual void initialize(int stage) override;
    virtual void finish() override;
};

}

#endif // ifndef APPLICATIONS_UDPAPP_WAICUDPSINK_H_

