/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 *
 * Copyright (C) 2021  Institute of Communication Networks (ComNets),
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

#include <omnetpp.h>
#include <cmath>
#include "TschSpectrumSensing.h"
#include "../TschHopping.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalNoise.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalReception.h"
#include "inet/physicallayer/common/packetlevel/BandListening.h"
#include "inet/common/INETMath.h"
#include "inet/common/Units.h"
#include "inet/common/ModuleAccess.h"

using namespace omnetpp;
using namespace inet;
using namespace inet::physicallayer;

namespace tsch {
namespace sixtisch {

Define_Module(TschSpectrumSensing);

void TschSpectrumSensing::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // we are within the 6top sublayer, which is between link and network layer.
    // therefore we take the later stage of the two
    if (stage == inet::InitStages::INITSTAGE_NETWORK_CONFIGURATION) {
        sensingTimer = new cMessage("timer-sensing");

        // get pointers to all the modules we (might) need
        auto radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        bandwidth = radioModule->par("bandwidth").doubleValue();
        radio = check_and_cast<IRadio*>(radioModule);
        radioMedium = check_and_cast<const IRadioMedium*>(radio->getMedium());
        analogModel = check_and_cast<const IAnalogModel*>(radioMedium->getAnalogModel());
        auto constCommunicationCache = check_and_cast<const ICommunicationCache*>(radioMedium->getCommunicationCache());
        communicationCache = const_cast<ICommunicationCache*>(constCommunicationCache);
        if (communicationCache == nullptr) {
            throw cRuntimeError("const_cast(): Cannot cast CommunicationCache");
        }
        ccaDetectionTime = SimTime(par("ccaDetectionTime").doubleValueInUnit("s"));
        channelPlan = check_and_cast<IChannelPlan*>(getModuleByPath("^.^.^.^.^.channelHopping"));
        measurementInterval = SimTime(par("measurementInterval").doubleValueInUnit("s"));
        sweepInterval = SimTime(par("sweepInterval").doubleValueInUnit("s"));
    }
}

void TschSpectrumSensing::handleMessage(cMessage *msg)
{
    if (msg == sensingTimer) {
        conductCca(lastChannel + channelPlan->getMinChannel());
        lastChannel++;
        lastChannel = lastChannel % (channelPlan->getMaxChannel()-channelPlan->getMinChannel());
        if (lastChannel > 0) {
            scheduleAt(simTime() + measurementInterval, sensingTimer);
        } else if (continous && lastChannel == 0) {
            scheduleAt(simTime() + sweepInterval, sensingTimer);
        }
    } else {
        EV << "TschSpectrumSesning Error: unknown self message received: " << msg << endl;
    }
}

void TschSpectrumSensing::emitSignal(int channel, double power)
{
    if (std::isnan(power)) {
        return;
    }

    std::string name = std::string("power-") + std::to_string(channel);

    if (registeredSignals.find(channel) == registeredSignals.end()) {
        auto statisticTemplate = getProperties()->get("statisticTemplate", "powerStats");
        getEnvir()->addResultRecorders(this, registerSignal(name.c_str()), name.c_str(), statisticTemplate);
        registeredSignals.insert(std::make_pair(channel, name));
    }

    emit(registerSignal(name.c_str()), power);
}

double TschSpectrumSensing::conductCca(int channel)
{
    Enter_Method_Silent();

    // we query all running transmissions (within the last ccaDetectionTime seconds)
    auto interferingTransmissions = const_cast<const std::vector<const ITransmission *> *>(communicationCache->computeInterferingTransmissions(radio, simTime()-ccaDetectionTime, simTime()));
    if (interferingTransmissions == nullptr) {
        throw cRuntimeError("const_cast(): Cannot cast return value of communicationCache->computeInterferingTransmissions(..)");
    }

    // no transmissions ongoing
    if (interferingTransmissions->empty()) {
        return NaN;
    }

    //auto receptionPowers = new std::vector<FunctionBase<WpHz, Domain<simsec, Hz>>*>();
    //const Ptr<const FunctionBase<WpHz, Domain<simsec, Hz>>> receptionPowers;
    auto receptionPowers = makeShared<SummedFunction<WpHz, Domain<simsec, Hz>>>();
    // extract power mapping of each transmission
    for (auto const& transmission : *interferingTransmissions) {
        auto rcpt = radioMedium->getReception(radio, transmission);
        auto dimRcpt = dynamic_cast<const DimensionalReception*>(rcpt);
        auto power = const_cast<const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>&>(dimRcpt->getPower());

        //std::cout << power->detailedInfo() << endl;
        if (power != nullptr) {
            //TODO: Not sure if correct?
            receptionPowers->addElement(power);
        } else {
            // alternatively just ignore a failure to cast here?
            throw cRuntimeError("const_cast(): Cannot cast ConstMapping");
        }
    }

//    Coord currentCoord;
//
//    auto mobilityModule = dynamic_cast<IMobility *>(getModuleByPath("^.^.^.^.mobility"));
//    if (mobilityModule != nullptr) {
//        currentCoord = mobilityModule->getCurrentPosition();
//    }
//
//    // add noise
//    auto backgroundNoise = radioMedium->getBackgroundNoise();
//    auto lstng = new BandListening(radio, simTime()-ccaDetectionTime, simTime(), currentCoord, currentCoord, Hz(TschHopping::frequency(channel)), Hz(bandwidth));
//    auto noise = backgroundNoise->computeNoise(lstng);
//    auto dimNoise = dynamic_cast<const DimensionalNoise*>(noise);
//    auto power = const_cast<ConstMapping *>(dimNoise->getPower());
//
//    if (power != nullptr) {
//        receptionPowers->push_back(power);
//    }

    // if there are transmissions ongoing, this should not happen
    // but is it an unrecoverable error? probably not
//    if (receptionPowers->empty()) {
//        return NaN;
//    }

    //auto listeningMapping = MappingUtils::createMapping(Argument::mapped_type(-150), DimensionSet(Dimension::time, Dimension::frequency), Mapping::LINEAR);

    // add up powers
    // TIL: Do not delete this afterwards as destructor of DimensionalNoise will delete it aswell oO
    //auto noisePower = new ConcatConstMapping<std::plus<double> >(listeningMapping, receptionPowers->begin(), receptionPowers->end(), false, Argument::MappedZero);

    // calculate total noise from transmissions taking into consideration the frequency domain as well
    auto dimensionalNoise = new DimensionalNoise(simTime()-ccaDetectionTime, simTime(), Hz(channelPlan->channelToCenterFrequency(channel)), Hz(bandwidth), receptionPowers);

    // get max power within given ccaDetectionTime
    // TODO is this the correct way to do it? would an average be an alternative?
    auto maxPower = math::mW2dBmW(dimensionalNoise->computeMaxPower(simTime()-ccaDetectionTime, simTime()).get());
//    if (receptionPowers->size() > 1) {
//        for (int i = 0; i < receptionPowers->size(); i++) {
//            receptionPowers->at(i)->print(std::cout, 0);
//            auto test = new DimensionalNoise(simTime()-ccaDetectionTime, simTime(), Hz(TschHopping::frequency(channel)), Hz(bandwidth), receptionPowers->at(i));
//            std::cout << test->computeMaxPower(simTime()-ccaDetectionTime, simTime()).get() << endl;
//            std::cout << test->computeMinPower(simTime()-ccaDetectionTime, simTime()).get() << endl;
//        }
//        noisePower->print(std::cout);
//        std::cout << "max " << maxPower << endl;
//    }

    delete dimensionalNoise;
    delete interferingTransmissions;
    //delete receptionPowers;

    emitSignal(channel, maxPower);
    return maxPower;
}

void TschSpectrumSensing::startContinousSensing()
{
    Enter_Method_Silent();

    continous = true;
    scheduleAt(simTime() + ccaDetectionTime, sensingTimer);
}

void TschSpectrumSensing::doSweep(int startChannel, int endChannel) {

    Enter_Method_Silent();

    continous = false;
    scheduleAt(simTime() + ccaDetectionTime, sensingTimer);
}

} // namespace sixtisch
} // namespace tsch
