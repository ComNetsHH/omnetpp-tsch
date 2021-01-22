// Copyright (C) 2021  Institute of Communication Networks (ComNets),
//                     Hamburg University of Technology (TUHH)
// Copyright (C) 2013  OpenSim Ltd.
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

#ifndef __INET_ISOTROPICDIMENSIONALBACKGROUNDNOISEWAIC_H
#define __INET_ISOTROPICDIMENSIONALBACKGROUNDNOISEWAIC_H

#include "inet/physicallayer/contract/packetlevel/IBackgroundNoise.h"
#include "inet/physicallayer/contract/packetlevel/INoise.h"
#include "inet/physicallayer/backgroundnoise/IsotropicDimensionalBackgroundNoise.h"
#include "inet/common/INETDefs.h"
#include "inet/common/INETUtils.h"
#include "inet/common/Units.h"
#include "inet/physicallayer/contract/packetlevel/IListening.h"

namespace tsch {

using namespace inet;
using namespace inet::physicallayer;

class IsotropicDimensionalBackgroundNoiseWaic : public IsotropicDimensionalBackgroundNoise {
  protected:
    WpHz powerSpectralDensity = WpHz(NaN);
    W power = W(NaN);
    mutable Hz bandwidth = Hz(NaN);

  protected:
    virtual void initialize(int stage) override;

  public:
    const INoise *computeNoise(const IListening *listening) const override;
    double getDistanceToAltimeter() const;
};

} // namespace inet

#endif // ifndef __INET_ISOTROPICDIMENSIONALBACKGROUNDNOISEWAIC_H

