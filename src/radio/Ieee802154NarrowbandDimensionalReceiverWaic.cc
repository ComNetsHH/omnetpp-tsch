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
#include "../mobility/FlexibleGridMobility.h"
#include "inet/mobility/static/StationaryMobility.h"
#include <math.h>

namespace tsch {

using namespace inet;
using namespace inet::physicallayer;
using namespace std;

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
    if (stage == INITSTAGE_LAST) {
        mac = check_and_cast<Ieee802154eMac*>(getModuleByPath("^.^.mac.mac"));
        EV_DETAIL << "Found MAC module - " << mac << endl;
    }

}


vector<tuple<Hz, Hz>> Ieee802154NarrowbandDimensionalReceiverWaic::getFrequenciesAffectedByAltimeter(
        simtime_t startTime, simtime_t endTime) const
{
    // TODO: implement chirp frequencies calculation based on the reception start, end times
    vector<tuple<Hz, Hz>> affectedFrequencies;
    tuple<Hz, Hz> exampleFreqRange = {Hz(0), Hz(0)};

    affectedFrequencies.push_back(exampleFreqRange);

    return affectedFrequencies;
}

W Ieee802154NarrowbandDimensionalReceiverWaic::getAltimeterInterferencePower(double distance, W txPower) const {
    // TODO implement
    return W(0);
}


// METHOD TO MODIFY TO INCLUDE RADIO ALTIMETER INTERFERNCE
bool Ieee802154NarrowbandDimensionalReceiverWaic::computeIsReceptionPossible(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part) const
{

    // This sanity check only verifies that the transmission is at least on the same frequency band
    if (!NarrowbandReceiverBase::computeIsReceptionPossible(listening, reception, part))
        return false;
    else {
        const FlatReceptionBase *flatReception = check_and_cast<const FlatReceptionBase *>(reception);

        auto currentFreq = mac ? mac->getCurrentFrequency() : Hz(0);
        auto distance = getDistanceToAltimeter();
        auto t_start = reception->getStartTime(part);
        auto t_end = reception->getEndTime(part);

        W minReceptionPower = flatReception->computeMinPower(t_start, t_end);

        auto affectedFrequencies = getFrequenciesAffectedByAltimeter(t_start, t_end);

        // If the altimeter chirp doesn't affect our current channel,
        // no need to calculate it's interference power
        if (isAltimeterInterfering(currentFreq, affectedFrequencies)) {
            auto altimeterInterferencePower = getAltimeterInterferencePower(distance, W(0));
            minReceptionPower = W(0); // TODO redefine depending on how altimeter interferes (just subtract?)
        }

        bool isReceptionPossible = minReceptionPower >= sensitivity;

        EV_DETAIL << "Computing whether reception is possible: minimum reception power = "
                << minReceptionPower << ", sensitivity = " << sensitivity << " -> reception is "
                << (isReceptionPossible ? "possible" : "impossible")
                << "\nReception duration from " << t_start << " to " << t_end
                << "\ncurrent frequency = " << currentFreq
                << ", distance to altimeter = " << distance << " m" << endl;

        return isReceptionPossible;
    }
}



bool Ieee802154NarrowbandDimensionalReceiverWaic::isAltimeterInterfering(Hz currentFreq, vector<tuple<Hz, Hz>> altimeterChirps) const
{
    for (auto chirp : altimeterChirps) {
        // check current frequency within chirp boundaries
        if (currentFreq > get<0>(chirp) && currentFreq < get<1>(chirp)) {
            EV_DETAIL << "Altimeter interference detected in range ("
                    << get<0>(chirp) << ", " << get<1>(chirp) << ")" << endl;
            return true;
        }

    }

    return false;
}

double Ieee802154NarrowbandDimensionalReceiverWaic::getDistanceToAltimeter() const {
    // Find NED network module
    auto simul = cModule::getParentModule()->getParentModule()->getParentModule()->getParentModule();

    // Find altimeter node (to be refactored with more efficient search later)
    cModule *altimeter;
    for (cModule::SubmoduleIterator it(simul); !it.end(); ++it) {
        altimeter = *it;
        std::string submFullName(altimeter->getFullName());

        if (submFullName.find(std::string("Altimeter")) != std::string::npos)
            break;
    }

    // In case no altimeter was found, the previous iterator will just return the last module,
    // so need to double-check
    std::string altiName(altimeter->getFullName());
    if (altiName.find(std::string("Altimeter")) == std::string::npos)
        return 0;

    auto altiMobility = check_and_cast<StationaryMobilityBase*> (altimeter->getSubmodule("mobility"));
    auto myMobility = check_and_cast<StationaryMobilityBase*> (cModule::getParentModule()->getParentModule()->getParentModule()->getSubmodule("mobility"));

    return myMobility->getCurrentPosition().distance(altiMobility->getCurrentPosition());
}

} // namespace tsch

