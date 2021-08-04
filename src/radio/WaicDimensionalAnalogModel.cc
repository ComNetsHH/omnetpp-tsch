//
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

#include "inet/physicallayer/analogmodel/packetlevel/DimensionalAnalogModel.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalNoise.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalReception.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalSnir.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalTransmission.h"
#include "inet/physicallayer/common/packetlevel/BandListening.h"
#include "inet/physicallayer/contract/packetlevel/IRadioMedium.h"
#include "WaicDimensionalAnalogModel.h"
#include "inet/mobility/static/StationaryMobility.h"

using namespace inet;

using namespace physicallayer;

namespace tsch {

Define_Module(WaicDimensionalAnalogModel);

void WaicDimensionalAnalogModel::initialize(int stage)
{
    DimensionalAnalogModelBase::initialize(stage);
    EV_DETAIL << "Initializing stage" << stage << endl;
    if (stage == INITSTAGE_LAST) {
        altimeterLocation = getAltimeterLocation();
        WATCH(altimeterLocation);
    }

}

const ISnir *WaicDimensionalAnalogModel::computeSNIR(const IReception *reception, const INoise *noise) const
{
    const DimensionalReception *dimensionalReception = check_and_cast<const DimensionalReception *>(reception);
    const DimensionalNoise *dimensionalNoise = check_and_cast<const DimensionalNoise *>(noise);

    auto distToAltimeter = reception->getEndPosition().distance(altimeterLocation);

    return new WaicDimensionalSnir(
            dimensionalReception,
            dimensionalNoise,
            distToAltimeter
            );
}

const INoise *WaicDimensionalAnalogModel::computeNoise(const IListening *listening, const IInterference *interference) const
{
    const BandListening *bandListening = check_and_cast<const BandListening *>(listening);
    Hz centerFrequency = bandListening->getCenterFrequency();
    Hz bandwidth = bandListening->getBandwidth();
    std::vector<Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>> receptionPowers;
    const DimensionalNoise *dimensionalBackgroundNoise = check_and_cast_nullable<const DimensionalNoise *>(interference->getBackgroundNoise());
    if (dimensionalBackgroundNoise) {
        const auto& backgroundNoisePower = dimensionalBackgroundNoise->getPower();
        receptionPowers.push_back(backgroundNoisePower);
    }
    const std::vector<const IReception *> *interferingReceptions = interference->getInterferingReceptions();
    for (const auto & interferingReception : *interferingReceptions) {
        const DimensionalReception *dimensionalReception = check_and_cast<const DimensionalReception *>(interferingReception);
        auto receptionPower = dimensionalReception->getPower();
        receptionPowers.push_back(receptionPower);
        EV_DETAIL << "Interference power begin " << endl;
        EV_DETAIL << *receptionPower << endl;
        EV_DETAIL << "Interference power end" << endl;
    }

    if (isAltimeterInterfering()) {
        // TODO: calculate altimeter power spectral density for given point in time?
        WpHz altimeterPowerSpectralDensity = WpHz(0.1);
        const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& altimeterPower =
                makeShared<ConstantFunction<WpHz, Domain<simsec, Hz>>>(altimeterPowerSpectralDensity);
        EV_DETAIL << "Detected altimeter interference, created power function: " << *altimeterPower << endl;
        receptionPowers.push_back(altimeterPower);
    }

    const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& noisePower = makeShared<SummedFunction<WpHz, Domain<simsec, Hz>>>(receptionPowers);

    EV_DETAIL << "Noise power begin " << endl;
    EV_DETAIL << *noisePower << endl;
    EV_DETAIL << "Noise power end" << endl;
    const auto& bandpassFilter = makeShared<Boxcar2DFunction<double, simsec, Hz>>(simsec(listening->getStartTime()), simsec(listening->getEndTime()), centerFrequency - bandwidth / 2, centerFrequency + bandwidth / 2, 1);
    return new DimensionalNoise(listening->getStartTime(), listening->getEndTime(), centerFrequency, bandwidth, noisePower->multiply(bandpassFilter));
}

const INoise *WaicDimensionalAnalogModel::computeNoise(const IReception *reception, const INoise *noise) const
{
    auto dimensionalReception = check_and_cast<const DimensionalReception *>(reception);
    auto dimensionalNoise = check_and_cast<const DimensionalNoise *>(noise);
    const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& noisePower = makeShared<AddedFunction<WpHz, Domain<simsec, Hz>>>(dimensionalReception->getPower(), dimensionalNoise->getPower());
    return new DimensionalNoise(reception->getStartTime(), reception->getEndTime(), dimensionalReception->getCenterFrequency(), dimensionalReception->getBandwidth(), noisePower);
}

const Coord& WaicDimensionalAnalogModel::getAltimeterLocation() const {
    // Find NED network module
    auto simul = cModule::getParentModule()->getParentModule();

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

    if (altiName.find(std::string("Altimeter")) == std::string::npos) {
        EV_WARN << "Altimeter not found, returning (0, 0, 0) position" << endl;
        return *(new Coord());
    }

    auto altiMobility = check_and_cast<StationaryMobilityBase*> (altimeter->getSubmodule("mobility"));
    auto altimeterLocation = new Coord(altiMobility->getCurrentPosition());

    return *altimeterLocation;
}



}



