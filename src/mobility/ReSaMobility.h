//
//  Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
//
//  Copyright (C) 2019  Institute of Communication Networks (ComNets),
//                      Hamburg University of Technology (TUHH)
//            (C) 2021  Gökay Apusoglu
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

#ifndef MOBILITY_RESAMOBILITY_H_
#define MOBILITY_RESAMOBILITY_H_

#include "inet/mobility/static/StationaryMobility.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/Quaternion.h"
#include "inet/common/geometry/common/GeographicCoordinateSystem.h"
#include "inet/visualizer/mobility/MobilityCanvasVisualizer.h"
#include <fstream>
#include <iostream>
#include <list>
#include <cstdio>
#include <cstring>
#include <regex>

using namespace inet;

namespace tsch {

class ReSaMobility : public inet::StationaryMobilityBase{

  public:
    ReSaMobility();
    virtual ~ReSaMobility();

  protected:
    virtual void setInitialPosition() override;
    virtual void setCoords(std::ifstream& inStream);
    virtual void setCoords(std::istringstream& inStream);

    // Type names and column indexes used in the layout file
    bool enableReadingFromFile = false;
    bool updateFromDisplayString = true;
    const char * ReSaLayoutPath;
    const char * moduleType;
    const char * coordList;

    double x_coord_offset;
    double y_coord_offset;

    int typeColidx;
    int xColidx;
    int yColidx;
    int zColidx;

};

} // namespace tsch


#endif /* MOBILITY_RESAMOBILITY_H_ */
