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

#include "WaicDimensionalSnir.h"

using namespace inet;

using namespace physicallayer;

WaicDimensionalSnir::WaicDimensionalSnir(const DimensionalReception *reception, const DimensionalNoise *noise, double distanceToAltimeter) :
    DimensionalSnir(reception, noise),
    distanceToAltimeter(distanceToAltimeter)
{
}

double WaicDimensionalSnir::computeMin() const
{
    // TODO: factor out common part
    EV_DEBUG << "Computing min WAIC SNIR, distance to altimeter " << distanceToAltimeter << endl;
    const DimensionalNoise *dimensionalNoise = check_and_cast<const DimensionalNoise *>(noise);
    const DimensionalReception *dimensionalReception = check_and_cast<const DimensionalReception *>(reception);
    EV_TRACE << "Reception power begin " << endl;
    EV_TRACE << *dimensionalReception->getPower() << endl;
    EV_TRACE << "Reception power end" << endl;
    // TODO: add altimeter interference to noise power?
    auto noisePower = dimensionalNoise->getPower();
    auto receptionPower = dimensionalReception->getPower();
    auto snir = receptionPower->divide(noisePower);
    simsec startTime = simsec(reception->getStartTime());
    simsec endTime = simsec(reception->getEndTime());
    Hz centerFrequency = dimensionalReception->getCenterFrequency();
    Hz bandwidth = dimensionalReception->getBandwidth();
    Point<simsec, Hz> startPoint(startTime, centerFrequency - bandwidth / 2);
    Point<simsec, Hz> endPoint(endTime, centerFrequency + bandwidth / 2);
    EV_TRACE << "SNIR begin " << endl;
    EV_TRACE << *snir << endl;
    EV_TRACE << "SNIR end" << endl;
    double minSNIR = snir->getMin(Interval<simsec, Hz>(startPoint, endPoint, 0b1, 0b0, 0b0));
    EV_DEBUG << "Computing minimum SNIR: start = " << startPoint << ", end = " << endPoint << " -> minimum SNIR = " << minSNIR << endl;
    return minSNIR;
}

double WaicDimensionalSnir::computeMax() const
{
    // TODO: factor out common part
    EV_DEBUG << "Computing max WAIC SNIR, distance to altimeter " << distanceToAltimeter << endl;
    const DimensionalNoise *dimensionalNoise = check_and_cast<const DimensionalNoise *>(noise);
    const DimensionalReception *dimensionalReception = check_and_cast<const DimensionalReception *>(reception);
    EV_DEBUG << "Reception power begin " << endl;
    EV_DEBUG <<* dimensionalReception->getPower() << endl;
    EV_DEBUG << "Reception power end" << endl;
    // TODO: add altimeter interference to noise power?
    auto noisePower = dimensionalNoise->getPower();
    auto receptionPower = dimensionalReception->getPower();
    auto snir = receptionPower->divide(noisePower);
    auto startTime = simsec(reception->getStartTime());
    auto endTime = simsec(reception->getEndTime());
    Hz centerFrequency = dimensionalReception->getCenterFrequency();
    Hz bandwidth = dimensionalReception->getBandwidth();
    Point<simsec, Hz> startPoint(startTime, centerFrequency - bandwidth / 2);
    Point<simsec, Hz> endPoint(endTime, centerFrequency + bandwidth / 2);
    EV_TRACE << "SNIR begin " << endl;
    EV_TRACE << *snir << endl;
    EV_TRACE << "SNIR end" << endl;
    double maxSNIR = snir->getMax(Interval<simsec, Hz>(startPoint, endPoint, 0b1, 0b0, 0b0));
    EV_DEBUG << "Computing maximum SNIR: start = " << startPoint << ", end = " << endPoint << " -> maximum SNIR = " << maxSNIR << endl;
    return maxSNIR;
}

double WaicDimensionalSnir::computeMean() const
{
    // TODO: factor out common part
    EV_DEBUG << "Computing mean WAIC SNIR, distance to altimeter " << distanceToAltimeter << endl;
    const DimensionalNoise *dimensionalNoise = check_and_cast<const DimensionalNoise *>(noise);
    const DimensionalReception *dimensionalReception = check_and_cast<const DimensionalReception *>(reception);
    EV_TRACE << "Reception power begin " << endl;
    EV_TRACE << *dimensionalReception->getPower() << endl;
    EV_TRACE << "Reception power end" << endl;
    auto noisePower = dimensionalNoise->getPower();
    // TODO: add altimeter interference to noise power?
    auto receptionPower = dimensionalReception->getPower();
    auto snir = receptionPower->divide(noisePower);
    auto startTime = simsec(reception->getStartTime());
    auto endTime = simsec(reception->getEndTime());
    Hz centerFrequency = dimensionalReception->getCenterFrequency();
    Hz bandwidth = dimensionalReception->getBandwidth();
    Point<simsec, Hz> startPoint(startTime, centerFrequency - bandwidth / 2);
    Point<simsec, Hz> endPoint(endTime, centerFrequency + bandwidth / 2);
    EV_TRACE << "SNIR begin " << endl;
    EV_TRACE << *snir << endl;
    EV_TRACE << "SNIR end" << endl;
    double meanSNIR = snir->getMean(Interval<simsec, Hz>(startPoint, endPoint, 0b1, 0b0, 0b0));
    EV_DEBUG << "Computing mean SNIR: start = " << startPoint << ", end = " << endPoint << " -> mean SNIR = " << meanSNIR << endl;
    return meanSNIR;
}


