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

#include "Ieee802154NarrowbandDimensionalReceiverWaic.h"
#include <math.h>

namespace tsch {

using namespace inet;
using namespace inet::physicallayer;

Define_Module(Ieee802154NarrowbandDimensionalReceiverWaic);

Ieee802154NarrowbandDimensionalReceiverWaic::Ieee802154NarrowbandDimensionalReceiverWaic() :
    Ieee802154NarrowbandDimensionalReceiver()
{
}

void Ieee802154NarrowbandDimensionalReceiverWaic::initialize(int stage)
{
    FlatReceiverBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        minInterferencePower = mW(math::dBmW2mW(par("minInterferencePower")));
    }
}

bool Ieee802154NarrowbandDimensionalReceiverWaic::computeIsReceptionPossible(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part) const
{
    auto d = getDistanceToAltimeter();
    //

    if (!NarrowbandReceiverBase::computeIsReceptionPossible(listening, reception, part))
        return false;
    else {
        const FlatReceptionBase *flatReception = check_and_cast<const FlatReceptionBase *>(reception);
        W minReceptionPower = flatReception->computeMinPower(reception->getStartTime(part), reception->getEndTime(part));
        bool isReceptionPossible = minReceptionPower >= sensitivity;
        EV_DEBUG << "Computing whether reception is possible: minimum reception power = " << minReceptionPower << ", sensitivity = " << sensitivity << " -> reception is " << (isReceptionPossible ? "possible" : "impossible") << endl;
        return isReceptionPossible;
    }
}

double Ieee802154NarrowbandDimensionalReceiverWaic::getDistanceToAltimeter() const {
    // Find highest NED network module
    auto simul = cModule::getParentModule()->getParentModule()->getParentModule()->getParentModule();

    // Find altimeter node and extract it's location
    cModule *subm;
    for (cModule::SubmoduleIterator it(simul); !it.end(); ++it) {
        subm = *it;
        std::string submFullName(subm->getFullName());

        if (submFullName.find(std::string("Altimeter")) != std::string::npos)
            break;
    }
    auto altimX = subm->getSubmodule("mobility")->par("initialX").doubleValue();
    auto altimY = subm->getSubmodule("mobility")->par("initialY").doubleValue();

    // Extract own coordinates as well
    auto hostNodeMobility = cModule::getParentModule()->getParentModule()->getParentModule()->getSubmodule("mobility");
    auto selfX = hostNodeMobility->par("initialX").doubleValue();
    auto selfY = hostNodeMobility->par("initialY").doubleValue();

    auto distanceToAltimeter = sqrt( pow(selfX - altimX, 2) + pow(selfY - altimY, 2) );

    EV_DETAIL << "Our distance to altimeter is - " << distanceToAltimeter << " m " << endl;
    EV_DETAIL << "Simtime - " << simTime() << endl;

    return distanceToAltimeter;
}

double Ieee802154NarrowbandDimensionalReceiverWaic::getAltimeterSpectralPower(simtime_t currentTime) {
    /** TODO: exact spectral density of an altimeter function implementation */
    return 0;
}

} // namespace tsch

