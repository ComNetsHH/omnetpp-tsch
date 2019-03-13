//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef LINKLAYER_IEEE802154E_IEEE802154EASN_H_
#define LINKLAYER_IEEE802154E_IEEE802154EASN_H_

#include <omnetpp.h>

class Ieee802154eASN
{
    private:
        int64_t asn;
        omnetpp::simtime_t reference;
        int64_t asn_reference;
        omnetpp::simtime_t macTsTimeslotLength;

    public:
        Ieee802154eASN();
        virtual ~Ieee802154eASN();
        int64_t getAsn(omnetpp::simtime_t at) const;
        int64_t getAsnReference() const;
        void setAsnReference(int64_t asnReference);
        omnetpp::simtime_t getReference() const;
        void setReference(omnetpp::simtime_t reference);
        const omnetpp::simtime_t& getMacTsTimeslotLength() const;
        void setMacTsTimeslotLength(const omnetpp::simtime_t& macTsTimeslotLength);
};

#endif /* LINKLAYER_IEEE802154E_IEEE802154EASN_H_ */
