#ifndef __FLEXIBLEGRIDMOBILITY_H
#define __FLEXIBLEGRIDMOBILITY_H

#include "inet/mobility/static/StaticGridMobility.h"

using namespace inet;

namespace tsch {

class FlexibleGridMobility : public inet::StaticGridMobility
{
  protected:
    virtual void setInitialPosition() override;
};

} // namespace tsch

#endif

