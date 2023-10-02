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

#ifndef APPLICATIONS_UDPAPP_SMOKEALARM_RESASMOKEUDPSINK_H_
#define APPLICATIONS_UDPAPP_SMOKEALARM_RESASMOKEUDPSINK_H_

#include "inet/common/INETDefs.h"
#include "inet/applications/udpapp/UdpSink.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/common/TimeTag_m.h"

using namespace inet;

namespace tsch {

class ResaSmokeUdpSink : public UdpSink
{
  public:
    ResaSmokeUdpSink();
    virtual ~ResaSmokeUdpSink();

  protected:
    virtual void processPacket(Packet *msg) override;
    std::map<L3Address, int> numReceivedPktsPerNeighbor;
};

}

#endif // ifndef APPLICATIONS_UDPAPP_SMOKEALARM_RESASMOKEUDPSINK_H_

