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

#ifndef LINKLAYER_CONTRACT_ICHANNELPLAN_H_
#define LINKLAYER_CONTRACT_ICHANNELPLAN_H_

#include "inet/common/Units.h"
#include <omnetpp.h>

namespace tsch {

using namespace inet;

class IChannelPlan {
  public:
    IChannelPlan() {}
    ~IChannelPlan() {}
    virtual std::vector<int> getChannels()
    {
        if (channelPlanIsContinuous()) {
            std::vector<int> channels;

            for (int i = 0; i < (getMaxChannel() - getMinChannel()); i++) {
                channels.push_back(i + getMinChannel());
            }

            return channels;

        } else {
            throw omnetpp::cRuntimeError("IChannelPlan: getChannels() not yet implemented for non-continuous channel plans");
        }

        return std::vector<int>();
    }

    virtual int getMinChannel() = 0;
    virtual int getMaxChannel() = 0;
    virtual units::values::Hz getMinCenterFrequency() = 0;
    virtual units::values::Hz getMaxCenterFrequency() = 0;
    virtual units::values::Hz getChannelSpacing() = 0;
    virtual bool channelPlanIsContinuous() { return true; };
    virtual units::values::Hz channelToCenterFrequency(int channel) = 0;
};

} // namespace tsch

#endif /* LINKLAYER_CONTRACT_ICHANNELPLAN_H_ */
