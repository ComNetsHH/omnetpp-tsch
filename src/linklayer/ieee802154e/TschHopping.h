/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright (C) 2019  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2019  Leo Krueger
 *           (C) 2004-2006 Andras Varga
 *           (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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

#ifndef LINKLAYER_IEEE802154E_TSCHHOPPING_H_
#define LINKLAYER_IEEE802154E_TSCHHOPPING_H_

#include <omnetpp/csimplemodule.h>
#include <cassert>

namespace tsch {

class TschHopping: public omnetpp::cSimpleModule
{
    protected:
        typedef std::vector<int> PatternVector;
    private:
        PatternVector pattern;
    public:
        TschHopping();
        virtual ~TschHopping();

        void initialize(int stage);

        int channel(int64_t asn, int channelOffset);
        double frequency(int channel);

        const PatternVector& getPattern() const {
            return pattern;
        }

        void setPattern(const PatternVector& pattern) {
            this->pattern = pattern;
        }
};

} // namespace tsch

#endif /* LINKLAYER_IEEE802154E_TSCHHOPPING_H_ */
