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

#ifndef _RESAOBSTACLELOSS_H
#define _RESAOBSTACLELOSS_H

#include <algorithm>

#include "inet/environment/contract/IPhysicalObject.h"
#include "inet/physicallayer/contract/packetlevel/ITracingObstacleLoss.h"
#include "inet/common/IVisitor.h"
#include "inet/common/figures/TrailFigure.h"
#include "inet/environment/contract/IPhysicalEnvironment.h"
#include "inet/physicallayer/base/packetlevel/TracingObstacleLossBase.h"
#include "inet/physicallayer/contract/packetlevel/IRadioMedium.h"

using namespace inet;
using namespace inet::physicallayer;

namespace tsch {

class ReSAObstacleLoss : public cModule, public IObstacleLoss
{
    protected:
      class TotalObstacleLossComputation : public IVisitor
      {
        protected:
          const ReSAObstacleLoss *obstacleLoss = nullptr;
          const Coord transmissionPosition;
          const Coord receptionPosition;
          mutable bool isObstacleFound_ = false;

        public:
          TotalObstacleLossComputation(const ReSAObstacleLoss *obstacleLoss, const Coord& transmissionPosition, const Coord& receptionPosition);
          void visit(const cObject *object) const override;
          bool isObstacleFound() const { return isObstacleFound_; }
      };
      /** @name Parameters */
      //@{
      /**
       * The radio medium where the radio signal propagation takes place.
       */
      IRadioMedium *medium = nullptr;
      /**
       * The physical environment that provides to obstacles.
       */
      physicalenvironment::IPhysicalEnvironment *physicalEnvironment = nullptr;
      //@}

  protected:
    double loss;
    virtual void initialize(int stage) override;
    virtual bool isObstacle(const physicalenvironment::IPhysicalObject *object, const Coord& transmissionPosition, const Coord& receptionPosition) const;

  public:
    ReSAObstacleLoss();
    virtual std::ostream& printToStream(std::ostream& stream, int level) const override;
    virtual double computeObstacleLoss(Hz frequency, const Coord& transmissionPosition, const Coord& receptionPosition) const override;
};

}
#endif

