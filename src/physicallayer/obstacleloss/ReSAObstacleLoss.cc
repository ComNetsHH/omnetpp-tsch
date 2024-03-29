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

#include "inet/common/ModuleAccess.h"
#include "inet/common/geometry/base/ShapeBase.h"
#include "inet/common/geometry/common/RotationMatrix.h"
#include "inet/common/geometry/object/LineSegment.h"
#include "inet/physicallayer/obstacleloss/IdealObstacleLoss.h"
#include "ReSAObstacleLoss.h"

using namespace inet;
using namespace inet::physicalenvironment;

namespace tsch {

Define_Module(ReSAObstacleLoss);

ReSAObstacleLoss::ReSAObstacleLoss()
{
}

void ReSAObstacleLoss::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        loss = par("loss");
        medium = check_and_cast<IRadioMedium *>(getParentModule());
        physicalEnvironment = getModuleFromPar<IPhysicalEnvironment>(par("physicalEnvironmentModule"), this);
    }
}

std::ostream& ReSAObstacleLoss::printToStream(std::ostream& stream, int level) const
{
    return stream << "ReSAObstacleLoss";
}

bool ReSAObstacleLoss::isObstacle(const IPhysicalObject *object, const Coord& transmissionPosition, const Coord& receptionPosition) const
{
    const ShapeBase *shape = object->getShape();
    const Coord& position = object->getPosition();
    const Quaternion& orientation = object->getOrientation();
    RotationMatrix rotation(orientation.toEulerAngles());
    const LineSegment lineSegment(rotation.rotateVectorInverse(transmissionPosition - position), rotation.rotateVectorInverse(receptionPosition - position));
    Coord intersection1, intersection2, normal1, normal2;
    bool hasIntersections = shape->computeIntersection(lineSegment, intersection1, intersection2, normal1, normal2);
    bool isObstacle = hasIntersections && intersection1 != intersection2;
//    if (isObstacle) {
//        ObstaclePenetratedEvent event(object, intersection1, intersection2, normal1, normal2, isObstacle ? 1 : 0);
//        const_cast<ReSAObstacleLoss *>(this)->emit(obstaclePenetratedSignal, &event);
//    }
    return isObstacle;
}

double ReSAObstacleLoss::computeObstacleLoss(Hz frequency, const Coord& transmissionPosition, const Coord& receptionPosition) const
{
    TotalObstacleLossComputation obstacleLossVisitor(this, transmissionPosition, receptionPosition);
    physicalEnvironment->visitObjects(&obstacleLossVisitor, LineSegment(transmissionPosition, receptionPosition));
    return obstacleLossVisitor.isObstacleFound() ? 1/pow(10,loss/10) : 1;
}

ReSAObstacleLoss::TotalObstacleLossComputation::TotalObstacleLossComputation(const ReSAObstacleLoss *obstacleLoss, const Coord& transmissionPosition, const Coord& receptionPosition) :
    obstacleLoss(obstacleLoss),
    transmissionPosition(transmissionPosition),
    receptionPosition(receptionPosition)
{
}

void ReSAObstacleLoss::TotalObstacleLossComputation::visit(const cObject *object) const
{
    if (!isObstacleFound_)
        isObstacleFound_ = obstacleLoss->isObstacle(check_and_cast<const IPhysicalObject *>(object), transmissionPosition, receptionPosition);
}

}


