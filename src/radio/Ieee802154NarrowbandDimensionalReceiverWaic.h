//
// Copyright (C) 2014 Florian Meier
// Copyright (C) 2013 OpenSim Ltd.
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

#ifndef __INET_IEEE802154NARROWBANDDIMENSIONALRECEIVERWAIC_H
#define __INET_IEEE802154NARROWBANDDIMENSIONALRECEIVERWAIC_H

#include "inet/physicallayer/base/packetlevel/FlatReceiverBase.h"
#include "inet/physicallayer/ieee802154/packetlevel/Ieee802154NarrowbandDimensionalReceiver.h"
#include "inet/physicallayer/base/packetlevel/ApskModulationBase.h"
#include "inet/physicallayer/base/packetlevel/FlatReceiverBase.h"
#include "inet/physicallayer/base/packetlevel/FlatReceptionBase.h"
#include "inet/physicallayer/base/packetlevel/FlatTransmissionBase.h"
#include "inet/physicallayer/base/packetlevel/NarrowbandNoiseBase.h"
#include "inet/physicallayer/common/packetlevel/ListeningDecision.h"
#include "inet/physicallayer/common/packetlevel/ReceptionDecision.h"
#include "inet/physicallayer/contract/packetlevel/IRadioMedium.h"
#include "inet/physicallayer/contract/packetlevel/SignalTag_m.h"
#include "../linklayer/ieee802154e/Ieee802154eMac.h"

namespace tsch {

using namespace inet;
using namespace inet::physicallayer;
using namespace std;
using namespace units::values;

class Ieee802154NarrowbandDimensionalReceiverWaic : public Ieee802154NarrowbandDimensionalReceiver
{
  protected:
    W minInterferencePower;
    W altimeterTxPower;
    Ieee802154eMac* mac;

  public:
    Ieee802154NarrowbandDimensionalReceiverWaic();

    void initialize(int stage) override;
    virtual bool computeIsReceptionPossible(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part) const override;

    /** @brief Calculate distance from container node to radio altimeter
     *
     *  @return distance to altimeter node in meters
     */
    double getDistanceToAltimeter() const;


    /** @brief Calculate power of the interfering altimeter signal at the receiver
     *
     *  @param distance distance to altimeter in meters
     *  @param txPower transmission power of altimeter in Watts
     *
     *  @return attenuated signal power of radio altimeter
     */
    W getAltimeterInterferencePower(double distance, W txPower) const;

    /**
     * @brief Get frequency range affected by altimeter chirp based on the reception timestamps
     *
     * @param startTime reception started
     * @param endTime reception ended
     *
     * @return vector of tuples, where each tuple is a pair <frequencyStart, frequencyEnd>
     * denoting frequency ranges affected by altimeter chirp
     *
     */
    vector<tuple<Hz, Hz>> getFrequenciesAffectedByAltimeter(simtime_t startTime, simtime_t endTime) const;

    /**
     * @brief Check whether altimeter interferes with current reception
     *
     * @param currentFreq frequency of the current cell / reception
     * @param altimeterChirps vector of tuples representing frequency ranges affected by
     * altimeter chirps
     *
     * @return true if current frequency falls under one of the altimeter chirps, false otherwise
     *
     */
    bool isAltimeterInterfering(Hz currentFreq, vector<tuple<Hz, Hz>> altimeterChirps) const;
};

} // namespace tsch

#endif // ifndef __INET_IEEE802154DIMENSIONALRECEIVER_H

