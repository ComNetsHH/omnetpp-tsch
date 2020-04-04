/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 *
 * Copyright (C) 2019  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2017  Lotte Steenbrink
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

#ifndef __TSCH_TSCHSPECTRUMSENSING_H_
#define __TSCH_TSCHSPECTRUMSENSING_H_

#include <omnetpp.h>
#include "inet/common/InitStages.h"
#include "inet/common/math/FunctionBase.h"
#include "inet/physicallayer/contract/packetlevel/IAnalogModel.h"
#include "inet/physicallayer/contract/packetlevel/ICommunicationCache.h"
#include "inet/physicallayer/contract/packetlevel/IRadio.h"
#include "inet/physicallayer/contract/packetlevel/IRadioMedium.h"
#include "../../contract/IChannelPlan.h"
#include "../TschHopping.h"

using namespace omnetpp;
using namespace inet::physicallayer;

namespace tsch {
namespace sixtisch {

/**
 * TODO - Generated class
 */
class TschSpectrumSensing : public cSimpleModule
{
private:
    IRadio *radio;
    ICommunicationCache *communicationCache;
    const IRadioMedium *radioMedium;
    const IAnalogModel *analogModel;
    std::map<int, std::string> registeredSignals;
    simtime_t ccaDetectionTime;
    double bandwidth;
    cMessage *sensingTimer;
    int lastChannel;
    bool continous;
    IChannelPlan *channelPlan;
    simtime_t measurementInterval;
    simtime_t sweepInterval;
protected:
    void emitSignal(int channel, double power);
public:
    TschSpectrumSensing() : cSimpleModule() , radio(nullptr), communicationCache(nullptr), radioMedium(nullptr), analogModel(nullptr), ccaDetectionTime(0), bandwidth(0), sensingTimer(nullptr), lastChannel(0), continous(false), channelPlan(nullptr), measurementInterval(0), sweepInterval(0) {
    }
    ~TschSpectrumSensing() {
        radio = nullptr;
        communicationCache = nullptr;
        radioMedium = nullptr;
        analogModel = nullptr;
        cancelAndDelete(sensingTimer);
        sensingTimer = nullptr;
    }

    virtual int numInitStages() const override {return inet::InitStages::INITSTAGE_NETWORK_CONFIGURATION + 1;}
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

    double conductCca(int channel);
    void startContinousSensing();

    void doSweep(int startChannel, int endChannel);
};
} // namespace sixtisch
} // namespace tsch

#endif
