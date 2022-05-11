/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright (C) 2021  Institute of Communication Networks (ComNets),
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
#include "../contract/IChannelPlan.h"
#include "inet/common/Units.h"

namespace tsch {

using namespace inet;

class TschHopping: public omnetpp::cSimpleModule, public IChannelPlan
{
    public:
        typedef std::vector<int> PatternVector;
        TschHopping();
        virtual ~TschHopping();

        void initialize(int stage);

        int channel(int64_t asn, int channelOffset);

        const PatternVector& getPattern() const {
            return pattern;
        }

        void setPattern(const PatternVector& pattern) {
            this->pattern = pattern;
        }

        const PatternVector& getBlacklistedChannels() const {
            return blacklistedChannels;
        }

        void setBlacklistedChannels(const PatternVector& blChannels) {
            this->blacklistedChannels = blChannels;
        }

        /** Get the lowest/highest channel number for the given frequency band (WAIC or ISM) */
        virtual int getMinChannel();
        virtual int getMaxChannel();

        int getNumChannels() {
            return this->numChannels;
        }

        virtual units::values::Hz getMinCenterFrequency();
        virtual units::values::Hz getMaxCenterFrequency();
        virtual units::values::Hz getChannelSpacing() { return units::values::Hz(5.0e6); }
        virtual units::values::Hz channelToCenterFrequency(int channel);
        virtual double channelToCenterFrequencyPlain(int channel);
        virtual bool isBlacklisted(int channelOffset);
        virtual int shiftBlacklisted(int channelOffset, int increment);
        PatternVector getHoppingSequence();

        std::string printPattern(PatternVector pv) {
            std::ostringstream out;
            for (auto el: pv)
                out << el << ", ";

            return out.str();
        }

        // TODO: why the heck does it not work??
//        friend std::ostream& operator<<(std::ostream& out, const PatternVector& pv) {
//            for (auto el: pv)
//                out << el << ", ";
//
//            return out;
//        }

    private:
        PatternVector pattern;
        PatternVector blacklistedChannels;
        units::values::Hz centerFrequency;
        int numChannels;

        virtual void removeBlacklistedChannels();
};

} // namespace tsch

#endif /* LINKLAYER_IEEE802154E_TSCHHOPPING_H_ */
