/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright (C) 2019  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2019  Leo Krueger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "Ieee802154eASN.h"
#include <cassert>

Ieee802154eASN::Ieee802154eASN()
    : asn(0)
    , reference(0)
    , asn_reference(0)
    , macTsTimeslotLength(0)
{
    // TODO Auto-generated constructor stub

}

int64_t Ieee802154eASN::getAsn(omnetpp::simtime_t at) const {
    double passed = SIMTIME_DBL(at - getReference());
    assert(passed >= 0);

    return (int64_t) ((passed / macTsTimeslotLength) + SIMTIME_DBL(reference));
}

int64_t Ieee802154eASN::getAsnReference() const {
    return asn_reference;
}

void Ieee802154eASN::setAsnReference(int64_t asnReference) {
    asn_reference = asnReference;
}

omnetpp::simtime_t Ieee802154eASN::getReference() const {
    return reference;
}

const omnetpp::simtime_t& Ieee802154eASN::getMacTsTimeslotLength() const {
    return macTsTimeslotLength;
}

void Ieee802154eASN::setMacTsTimeslotLength(
        const omnetpp::simtime_t& macTsTimeslotLength) {
    this->macTsTimeslotLength = macTsTimeslotLength;
}

void Ieee802154eASN::setReference(omnetpp::simtime_t reference) {
    this->reference = reference;
}

Ieee802154eASN::~Ieee802154eASN() {
    // TODO Auto-generated destructor stub
}

