#ifndef __FLEXIBLEGRIDMOBILITY_H
#define __FLEXIBLEGRIDMOBILITY_H

#include "inet/mobility/static/StaticGridMobility.h"

using namespace inet;

namespace tsch {

class FlexibleGridMobility : public inet::StaticGridMobility
{
  protected:
    virtual void setInitialPosition() override;
    virtual void initialize(int stage) override;

    Coord* rotateAroundPoint(Coord target, Coord origin);
    Coord* getOriginCoordinates(Coord topLeftCorner, Coord bottomRightCorner);

};

} // namespace tsch

#endif

