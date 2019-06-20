/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright (C) 2019  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2019  Leo Krueger
 *           (C) 2004-2006 Andras Varga
 *           (C) 2000  Institut fuer Telematik, Universitaet Karlsruhe
 *           (C) 2000-2001 Jochen Reber, Vincent Oberle
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

#include <algorithm>
#include <sstream>

#include "inet/common/INETUtils.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/PatternMatcher.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "TschLink.h"
#include "TschSlotframe.h"
#include "inet/networklayer/ipv4/RoutingTableParser.h"
#include "../../common/TschSimsignals.h"
#include "inet/common/Simsignals.h"

// TODO from http://rootdirectory.ddns.net/dokuwiki/doku.php?id=software:sstr
#define SSTR( x ) static_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

namespace tsch {

using namespace utils;

Define_Module(TschSlotframe);

std::ostream& operator<<(std::ostream& os, const TschLink& e)
{
    os << e.str();
    return os;
};

TschSlotframe::~TschSlotframe()
{
    for (auto & elem : links)
        delete elem;
}

void TschSlotframe::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        macSlotframeSize = par("macSlotframeSize");
        macSlotframeHandle = par("macSlotframeHandle");

        cComponent::registerSignal("linkChangedSignal");

        WATCH_PTRVECTOR(links);
    }
}

void TschSlotframe::refreshDisplay() const
{
    char buf[80];
    sprintf(buf, "%d links", (int)links.size());
    getDisplayString().setTagArg("t", 0, buf);
}

void TschSlotframe::handleMessage(cMessage *msg)
{
    throw cRuntimeError("This module doesn't process messages");
}

void TschSlotframe::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    if (getSimulation()->getContextType() == CTX_INITIALIZE)
        return; // ignore notifications during initialize

    Enter_Method_Silent();
    //printSignalBanner(signalID, obj, details); TODO
}

void TschSlotframe::printSlotframe() const
{
    EV << "-- Slotframe " << macSlotframeHandle << ", size " << macSlotframeSize << " --" << endl;
    EV << stringf("%-16s %-16s %-16s %-16s %-16s\n",
            "slotOffset", "channelOffset", "type", "options", "addr");

    for (int i = 0; i < getNumLinks(); i++) {
        TschLink *link = getLink(i);
        EV << stringf("%-16s %-16s %-16s %-16s %-16s \n",
                SSTR(link->getSlotOffset()).c_str(),
                SSTR(link->getChannelOffset()).c_str(),
                (std::string(link->isNormal()?"NORM ":"") + std::string(link->isAdv()?"ADV ":"")).c_str(),
                (std::string(link->isRx()?"RX ":"") + std::string(link->isTx()?"TX ":"") + std::string(link->isShared()?"SHARE":"")).c_str(),
                link->getAddr().str().c_str());
    }
    EV << "\n";
}

void TschSlotframe::purge()
{
    // purge unicast routes
    for (auto it = links.begin(); it != links.end(); ) {
        TschLink *link = *it;
        if (link->isValid())
            ++it;
        else {
            it = links.erase(it);
            ASSERT(link->getSlotframe() == this);    // still filled in, for the listeners' benefit
            emit(linkDeletedSignal, link);
            delete link;
        }
    }
}

TschLink *TschSlotframe::getLink(int k) const
{
    if (k < (int)links.size())
        return links[k];
    return nullptr;
}

std::vector<MacAddress> TschSlotframe::getMacDedicated(){
    std::vector<MacAddress> tempMacDedicated;
    for(int i = 0; i < (int)links.size(); i++){
        if(!(links[i]->isShared())){
            bool found = false;
            for (int j = 0; j < (int)tempMacDedicated.size(); j++){
                if(tempMacDedicated[j] == links[i]->getAddr()){
                    found = true;
                    break;
                }
            }
            if(!found){
                tempMacDedicated.push_back(links[i]->getAddr());
            }
        }
    }
    return tempMacDedicated;
}

// The 'routes' vector stores the routes in this order.
// The best matching route should precede the other matching routes,
// so the method should return true if a is better the b.
bool TschSlotframe::linkLessThan(const TschLink *a, const TschLink *b) const
{
    // smaller metric is better
    return a->getSlotOffset() < b->getSlotOffset();
}

void TschSlotframe::internalAddLink(TschLink *entry)
{


    // The 'routes' vector may contain multiple routes with the same destination/netmask.
    // Routes are stored in descending netmask length and ascending administrative_distance/metric order,
    // so the first matching is the best one.

    // add to tables
    // we keep entries sorted by netmask desc, metric asc in routeList, so that we can
    // stop at the first match when doing the longest netmask matching
    auto pos = upper_bound(links.begin(), links.end(), entry, LinkLessThan(*this));
    links.insert(pos, entry);
    entry->setSlotframe(this);
}

void TschSlotframe::addLink(TschLink *entry)
{
    Enter_Method("addLink(...)");
    // This method should be called before calling entry->str()
    internalAddLink(entry);
    EV_INFO << "add link " << entry->str() << "\n";
    emit(linkAddedSignal, entry);
}

TschLink *TschSlotframe::internalRemoveLink(TschLink *entry)
{
    auto i = std::find(links.begin(), links.end(), entry);
    if (i != links.end()) {
        links.erase(i);
        return entry;
    }
    return nullptr;
}

TschLink *TschSlotframe::removeLink(TschLink *entry)
{
    Enter_Method("removeLink(...)");

    entry = internalRemoveLink(entry);

    if (entry != nullptr) {
        EV_INFO << "remove link " << entry->str() << "\n";
        ASSERT(entry->getSlotframe() == this);    // still filled in, for the listeners' benefit
        emit(linkDeletedSignal, entry);
        entry->setSlotframe(nullptr);
    }
    return entry;
}

bool TschSlotframe::deleteLink(TschLink *entry)    //TODO this is almost duplicate of removeRoute()
{
    Enter_Method("deleteLink(...)");

    entry = internalRemoveLink(entry);

    if (entry != nullptr) {
        EV_INFO << "delete link " << entry->str() << "\n";
        ASSERT(entry->getSlotframe() == this);    // still filled in, for the listeners' benefit
        emit(linkDeletedSignal, entry);
        delete entry;
    }
    return entry != nullptr;
}

void TschSlotframe::linkChanged(TschLink *entry, int fieldCode)
{
    if (fieldCode == TschLink::F_CHANOFF) {    // our data structures depend on these fields
        entry = internalRemoveLink(entry);
        ASSERT(entry != nullptr);
        internalAddLink(entry);
    }
    emit(linkChangedSignal, entry);    // TODO include fieldCode in the notification
}

bool TschSlotframe::handleOperationStage(LifecycleOperation *operation, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();
    // TODO:
//    int stage = operation->getCurrentStage();
//    if (dynamic_cast<ModuleStartOperation *>(operation)) {
//        if (static_cast<ModuleStartOperation::Stage>(stage) == ModuleStartOperation::STAGE_NETWORK_LAYER) {
//            // read routing table file (and interface configuration)
//        }
//        else if (static_cast<ModuleStartOperation::Stage>(stage) == ModuleStartOperation::STAGE_TRANSPORT_LAYER) {
//
//        }
//    }
//    else if (dynamic_cast<ModuleStopOperation *>(operation)) {
//        if (static_cast<ModuleStopOperation::Stage>(stage) == ModuleStopOperation::STAGE_NETWORK_LAYER) {
//
//        }
//    }
//    else if (dynamic_cast<ModuleCrashOperation *>(operation)) {
//        if (static_cast<ModuleCrashOperation::Stage>(stage) == ModuleCrashOperation::STAGE_CRASH) {
//            while (!links.empty())
//                delete removeLink(links[0]);
//        }
//    }
    return true;
}

TschLink *TschSlotframe::createNewLink()
{
    return new TschLink();
}

/**
 * Returns iterator to the link scheduled for the ASN given
 * or end() if nothing is scheduled.
 */
std::vector<TschLink *>::iterator TschSlotframe::getLinkFromASNInternal(int64_t asn)
{
    for (auto it = links.begin(); it != links.end(); ++it) {
        if ((*it)->getSlotOffset() == getOffsetFromASN(asn)) {
            return it;
            break;
        }
        if ((*it)->getSlotOffset() > getOffsetFromASN(asn))
            break;
    }

    return links.end();
}

/**
 * Returns iterator to the link scheduled for the ASN given
 * or end() if nothing is scheduled.
 */
std::vector<TschLink *>::iterator TschSlotframe::getNextLinkFromASNInternal(int64_t asn)
{
    for (auto it = links.begin(); it != links.end(); ++it) {
        if ((*it)->getSlotOffset() > getOffsetFromASN(asn))
            return it;
    }

    return links.begin();
}

/**
 * Returns the link scheduled for the ASN given
 * or null if nothing is scheduled.
 */
TschLink *TschSlotframe::getLinkFromASN(int64_t asn)
{
    auto it = getLinkFromASNInternal(asn);

    if (it != links.end())
        return *it;
    else
        return nullptr;
}

/**
 * Get the next scheduled link considering the given ASN.
 */
TschLink *TschSlotframe::getNextLink(int64_t asn)
{
    return *getNextLinkFromASNInternal(asn);
}

/**
 * Get the ASN of the next scheduled link.
 * Useful to suspend execution of MAC until the next scheduled link.
 */
int64_t TschSlotframe::getASNofNextLink(int64_t asn)
{
    auto l = getNextLink(asn);

    // we had a wrap around within the slotframe
    if (l->getSlotOffset() < getOffsetFromASN(asn)) {
        return asn + l->getSlotOffset() + (macSlotframeSize - getOffsetFromASN(asn));
    }

    // exactly one slotframe later
    else if (l->getSlotOffset() == getOffsetFromASN(asn)) {
        return asn + macSlotframeSize;
    }

    // after current slotOffset but no wrap-around
    else if (l->getSlotOffset() > getOffsetFromASN(asn)) {
        return asn + (l->getSlotOffset() - getOffsetFromASN(asn));
    }

    return -1; // should never happen
}

} // namespace inet

