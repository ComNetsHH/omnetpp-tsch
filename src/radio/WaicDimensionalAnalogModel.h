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

#ifndef __WAICDIMENSIONALANALOGMODEL_H
#define __WAICDIMENSIONALANALOGMODEL_H

#include "inet/physicallayer/analogmodel/packetlevel/DimensionalAnalogModel.h"
// FL_TEST
//#include "inet/physicallayer/analogmodel/packetlevel/DimensionalSnir.h"

#include "WaicDimensionalSnir.h"

namespace tsch {

using namespace inet;

using namespace physicallayer;

class WaicDimensionalAnalogModel : public DimensionalAnalogModel
{
  protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return NUM_INIT_STAGES; };

  private:
    Coord altimeterLocation;
    std::vector<Coord> V_AltimeterLocation;
    std::vector<simtime_t> RaOffSet;
    std::vector<double> Ra_Freq_OffSet;
    double T_chirp;
    double f_chirp_min;
    double f_chirp_max;
    //int numAltimeters;
    //std::vector<Coord> V_AltimeterLocation_test;
  public:

//    WaicDimensionalAnalogModel(const DimensionalReception *reception);

    virtual const ISnir *computeSNIR(const IReception *reception, const INoise *noise) const override;

    virtual const INoise *computeNoise(const IListening *listening, const IInterference *interference) const override;
    virtual const INoise *computeNoise(const IReception *reception, const INoise *noise) const override;

    const Coord& getAltimeterLocation() const;
    const std::vector<Coord>  getAltimeterLocation_v() const;
    const std::vector<Coord>  getAltimeterLocation_v_str() const;
    const std::vector<simtime_t>  getRaOffSet() const;
    const std::vector<double>  getRa_Freq_OffSet() const;


    // TODO: Implement logic to check whether we have to consider altimeter interference
    // at given point in time / frequency
    // DONE FL
    const bool isAltimeterInterfering(Hz centerFrequency,  Hz bandwidth, simsec startTime, simsec endTime, double RA_OffSet, double RA_Freq_OffSet) const; // { return uniform(0, 1) > 0.1; };

    // const tuple<double , double> computeTimeInt(const IReception *reception) const;
    const std::tuple<double , double> computeTimeInt() const;
    const double AltimeterInterferingPower(double distance) const;

};

}


#endif // ifndef __WAICDIMENSIONALANALOGMODEL_H

