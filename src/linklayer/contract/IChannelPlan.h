/*
 * IChannelPlan.h
 *
 *  Created on: 06.09.2019
 *      Author: leo
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
