/*
 * TschNeighbor.h
 *
 *  Created on: 12.03.2019
 *      Author: leo
 */

#ifndef LINKLAYER_IEEE802154E_TSCHNEIGHBOR_H_
#define LINKLAYER_IEEE802154E_TSCHNEIGHBOR_H_

#include "inet/linklayer/common/MacAddress.h"

class TschNeighbor
{
    private:
        inet::MacAddress addr;
        bool broadcast;
        bool timesource; // TODO unused

    public:
        TschNeighbor();
        virtual ~TschNeighbor();
};

#endif /* LINKLAYER_IEEE802154E_TSCHNEIGHBOR_H_ */
