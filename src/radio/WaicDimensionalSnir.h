/*
 * WaicDimensionalSnir.h
 *
 *  Created on: 28 Jul 2021
 *      Author: yevhenii
 */

#ifndef RADIO_WAICDIMENSIONALSNIR_H_
#define RADIO_WAICDIMENSIONALSNIR_H_

#include "inet/physicallayer/analogmodel/packetlevel/DimensionalSnir.h"

using namespace inet;

using namespace physicallayer;

class WaicDimensionalSnir : public DimensionalSnir
{
  public:
    WaicDimensionalSnir(const DimensionalReception *reception, const DimensionalNoise *noise, double distanceToAltimeter);

  private:
    double distanceToAltimeter;

  protected:
    virtual double computeMin() const;
    virtual double computeMax() const;
    virtual double computeMean() const;
};



#endif /* RADIO_WAICDIMENSIONALSNIR_H_ */
