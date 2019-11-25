/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright (C) 2019  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2019  Louis Yin
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

#include "TschNeighbor.h"
#include "inet/common/INETUtils.h"
#include <iostream>

namespace tsch{

Define_Module(TschNeighbor);

TschNeighbor::~TschNeighbor() {

}

void TschNeighbor::initialize(int stage){
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL){
        this->dedicated = false;
        setMethod(par("method"));
        this->queueLength = par("queueLength");
        this->currentVirtualLinkIDKey = -2;
        this->enableSelectDedicated = par("enableSelectDedicated");
        this->enablePriorityQueue = par("enablePriorityQueue");
    }

}

void TschNeighbor::handleMessage(cMessage *msg) {
    throw cRuntimeError("This module doesn't process messages");
}
// New virtual queue
bool TschNeighbor::add2Queue(Packet *packet,MacAddress macAddr, int virtualLinkID) {
    if(!this->enablePriorityQueue && (virtualLinkID == -1)){
        virtualLinkID = 0;
    }
    bool added = false;
    auto searchMacAddress = this->macToQueueMap.find(macAddr);
    if(searchMacAddress != macToQueueMap.end()){
        EV_DETAIL << "[TschNeighbor] The given Macaddress: " << macAddr << " is already a Neighbor." << endl;
    }else{
        EV_DETAIL << "[TschNeighbor] The given Macaddress: " << macAddr << " is not found. New entry added in Neighbor." << endl;
        this->macToQueueMap.insert(std::make_pair(macAddr, createVirtualQueue()));
    }
    //this->macToQueueMap.insert(std::make_pair(macAddr, createVirtualQueue()));
    // This if clause might be useless
    searchMacAddress = this->macToQueueMap.find(macAddr);
    if(searchMacAddress != macToQueueMap.end()){
        searchMacAddress->second->insert(std::make_pair(virtualLinkID, createQueue()));
        auto searchVirtualQueue = searchMacAddress->second->find(virtualLinkID);
        auto virtualQueueSize = (int)(searchVirtualQueue->second->size());
        EV_DETAIL << "[TschNeighbor] The current queue for this Neighbor is: " << virtualQueueSize << endl;
        if(virtualQueueSize< this->queueLength){
            searchVirtualQueue->second->push_back(packet);
            EV_DETAIL << "[TschNeighbor] Packet is added to the queue." << endl;
            added = true;
        }else{
            EV_DETAIL << "[TschNeighbor] The queue of this Neighbor is full." << endl;
        }
    }
    if(added){
        backoffTable.insert(std::make_pair(macAddr, new TschCSMA(par("macMinBE"),par("macMaxBE"))));
    }
    return added;
}

int TschNeighbor::checkQueueSizeAt(MacAddress macAddress) {
    auto search = this->macToQueueMap.find(macAddress);
    if(search != this->macToQueueMap.end()){
        int queueSize= 0;
        for(auto itr = search->second->begin(); itr != search->second->end(); ++itr){
            queueSize += (int)itr->second->size();
        }
        return queueSize;
    }else{
        EV_DETAIL << "[TschNeighbor] This Neighbor does not exist." << endl;
        return 0;
    }
}

int TschNeighbor::checkVirtualQueueSizeAt(MacAddress macAddress,int virtualLinkID){
    auto search = this->macToQueueMap.find(macAddress);
    if(search != this->macToQueueMap.end()){
        auto virtualQueue = search->second;
        if(virtualLinkID == -1 || virtualLinkID == 0){
            if(virtualQueue->find(-1) != virtualQueue->end() || virtualQueue->find(0) != virtualQueue->end()){
                if (virtualLinkID == -1) {
                    int prioritySize = 0;
                    auto priorityQueue = virtualQueue->find(-1);
                    if (priorityQueue != virtualQueue->end()) {
                        prioritySize = priorityQueue->second->size();
                    }
                    return prioritySize;
                } else{
                    int normalSize = 0;
                    int prioritySize = 0;
                    auto normalQueue = virtualQueue->find(0);
                    if (normalQueue != virtualQueue->end()) {
                        normalSize = normalQueue->second->size();
                    }
                    auto priorityQueue = virtualQueue->find(-1);
                    if (priorityQueue != virtualQueue->end()) {
                        prioritySize = priorityQueue->second->size();
                    }
                    return (prioritySize + normalSize);
                }
            }
        }else{
            if(virtualQueue->find(virtualLinkID) != virtualQueue->end()){
                return ((int)(virtualQueue->find(virtualLinkID)->second->size()));
            }
        }
        EV_DETAIL << "[TschNeighbor] The virtual link ID does not exist." << endl;
    }else{
        EV_DETAIL << "[TschNeighbor] The Neighbor does not exist." << endl;
    }
    return 0;
}




int TschNeighbor::checkTotalQueueSize() {
    int total = 0;
    for (auto itrOuter = this->macToQueueMap.begin(); itrOuter != this->macToQueueMap.end(); ++itrOuter) {
        for(auto itrInner = itrOuter->second->begin(); itrInner != itrOuter->second->end(); ++itrInner) {
                total += (int)itrInner->second->size();
        }
    }
    EV_DETAIL << "[TschNeighbor] The number of total packets at this node is: "<< total <<"." << endl;
    return total;
}


int TschNeighbor::getCurrentNeighborQueueSize(){
    int queueSize = 0;
    auto search = this->macToQueueMap.find(this->currentNeighborKey);
    if(search != this->macToQueueMap.end()){
        for(auto itr = search->second->begin(); itr != search->second->end(); ++itr){
            queueSize += (int)itr->second->size();
        }
        EV_DETAIL << "[TschNeighbor] The queue size for the current neighbor is: " << queueSize << endl;
        return queueSize;
    }else{
        EV_DETAIL << "[TschNeighbor] This Neighbor does not exist." << endl;
        return queueSize;
    }
}

inet::Packet* TschNeighbor::getCurrentNeighborQueueFirstPacket(){
    return this->macToQueueMap.find(this->currentNeighborKey)->second->find(this->currentVirtualLinkIDKey)->second->front();
}
void TschNeighbor::removeFirstPacketFromQueue(){
    auto searchNeighbor = this->macToQueueMap.find(this->currentNeighborKey);
    auto searchVirtualQueue = searchNeighbor->second->find(this->currentVirtualLinkIDKey);
    if((int)searchVirtualQueue->second->size()>0){
        searchVirtualQueue->second->pop_front();
        EV_DETAIL << "[Remove] The packet has been successfully removed." << endl;
    }
}



TschCSMA* TschNeighbor::getCurrentTschCSMA(){
    return this->backoffTable.find(currentNeighborKey)->second;
}

void TschNeighbor::failedTX(){
    this->getCurrentTschCSMA()->failedTX(this->isDedicated());
}

void TschNeighbor::terminateCurrentTschCSMA(){
    this->getCurrentTschCSMA()->terminate();
}

void TschNeighbor::reset() {
    this->addr.setAddress(inet::MacAddress::UNSPECIFIED_ADDRESS.str().c_str());
    this->dedicated = false;
    this->currentVirtualLinkIDKey = -2;
    this->currentNeighborKey.setAddress(inet::MacAddress::UNSPECIFIED_ADDRESS.str().c_str());
}
void TschNeighbor::setDedicated(bool value){
    this->dedicated = value;
}

bool TschNeighbor::checkAndselectQueue(std::vector<inet::MacAddress> tempMacDedicated,TschSlotframe* sf){
    bool found = false;
    switch (this->method) {
    // Iterate over all neighbors and select the first that matches(shared link, not in backoff, packetqueue is not empty)
    case 1: {
        inet::MacAddress potentialNeighbor;
        for (auto itr = this->backoffTable.begin(); itr != this->backoffTable.end(); ++itr) {
            bool match = false;
            for (int j = 0; j < (int) tempMacDedicated.size(); j++) {
                if (itr->first == tempMacDedicated[j] && this->enableSelectDedicated) {
                    // A potential dedicated Neighbor is already selected in case no shared neighbor is found
                    if (potentialNeighbor.isUnspecified()) {
                        auto virtualQueue = this->macToQueueMap.find(itr->first)->second;
                        int normalSize = 0;
                        int prioritySize = 0;
                        auto normalQueue = virtualQueue->find(0);
                        if (normalQueue != virtualQueue->end()) {
                            normalSize = normalQueue->second->size();
                        }
                        auto priorityQueue = virtualQueue->find(-1);
                        if (priorityQueue != virtualQueue->end()) {
                            prioritySize = priorityQueue->second->size();
                        }
                        if ((itr->second->getRandomNumber() == 0) && ((prioritySize + normalSize) > 0)) {
                            potentialNeighbor = itr->first;
                        }
                    }
                    match = true;
                    break;
                }
            }
            // Shared neighbor is found
            if (!match) {
                auto virtualQueue = this->macToQueueMap.find(itr->first)->second;
                int normalSize = 0;
                int prioritySize = 0;

                auto normalQueue = virtualQueue->find(0);
                if (normalQueue != virtualQueue->end()) {
                    normalSize = normalQueue->second->size();
                }
                auto priorityQueue = virtualQueue->find(-1);
                if (priorityQueue != virtualQueue->end()) {
                    prioritySize = priorityQueue->second->size();
                }

                if ((itr->second->getRandomNumber() == 0) && ((prioritySize + normalSize) > 0)) {
                    // 0 is the virtualLinkID for default
                    this->setSelectedQueue(itr->first,0);
                    found = true;
                    break;
                }
            }
        }
        // No shared Neighbor is found, therefore a dedicated neighbor with packets is selected
        if ((!found) && (!potentialNeighbor.isUnspecified()) && this->enableSelectDedicated){
            this->setSelectedQueue(potentialNeighbor,0);
            found = true;
        }
        break;
    }
    case 2: {
        inet::MacAddress temp_position;
        int temp_longest = 0;
        //potential Neighbor in case no shared slot is found
        inet::MacAddress temp_potentialPosition;
        int temp_potentialLongest = 0;
        for (auto itr = this->backoffTable.begin(); itr != this->backoffTable.end(); ++itr) {
            bool match = false;
            for (int j = 0; j < (int) tempMacDedicated.size(); j++) {
                if (itr->first == tempMacDedicated[j]) {
                    // Prevent unnecessary checks
                    if (itr->first > temp_potentialPosition && this->enableSelectDedicated) {
                        if (itr->second->getRandomNumber() == 0) {
                            auto virtualQueue = this->macToQueueMap.find(itr->first)->second;
                            int tempVirtualQueueSizeDefault = 0;
                            int tempVirtualQueueSizePriority = 0;
                            auto tempNormalQueue = virtualQueue->find(0);
                            if(tempNormalQueue != virtualQueue->end()){
                                tempVirtualQueueSizeDefault = tempNormalQueue->second->size();
                            }
                            auto tempPriorityQueue = virtualQueue->find(-1);
                            if(tempPriorityQueue != virtualQueue->end()){
                                tempVirtualQueueSizePriority = tempPriorityQueue->second->size();
                            }
                            if ((tempVirtualQueueSizeDefault + tempVirtualQueueSizePriority) > temp_potentialLongest ) {
                            temp_potentialLongest = tempVirtualQueueSizeDefault +tempVirtualQueueSizePriority;
                            temp_potentialPosition = itr->first;
                            }
                        }
                    }
                    match = true;
                    break;
                }
            }
            if (!match) {
                if (itr->second->getRandomNumber() == 0) {
                    auto virtualQueue = this->macToQueueMap.find(itr->first)->second;
                    int tempVirtualQueueSizeDefault = 0;
                    int tempVirtualQueueSizePriority = 0;
                    auto tempNormalQueue = virtualQueue->find(0);
                    if(tempNormalQueue != virtualQueue->end()){
                        tempVirtualQueueSizeDefault = tempNormalQueue->second->size();
                    }
                    auto tempPriorityQueue = virtualQueue->find(-1);
                    if(tempPriorityQueue != virtualQueue->end()){
                        tempVirtualQueueSizePriority = tempPriorityQueue->second->size();
                    }
                    if ((tempVirtualQueueSizeDefault + tempVirtualQueueSizePriority) > temp_longest) {
                        temp_longest =tempVirtualQueueSizeDefault + tempVirtualQueueSizePriority;
                        temp_position = itr->first;
                        found = true;
                    }

                }
            }
        }
        if(found){
            this->setSelectedQueue(temp_position,0);
        }else{
            if (!temp_potentialPosition.isUnspecified()&& this->enableSelectDedicated) {
                this->setSelectedQueue(temp_potentialPosition,0);
                found = true;
            }
        }
        break;
    }
    default: {
        break;
    }
    }
    return found;
}

void TschNeighbor::setSelectedQueue(MacAddress macAddr, int linkID) {
    this->currentNeighborKey = macAddr;
    if(linkID == 0 ){
        int prioritySize = 0;
        auto priorityQueue = this->macToQueueMap.find(this->currentNeighborKey)->second->find(-1);
        if(priorityQueue != this->macToQueueMap.find(this->currentNeighborKey)->second->end()){
            prioritySize = (int)(priorityQueue->second->size());
        }
        if(this->macToQueueMap.find(this->currentNeighborKey)->second->count(-1) && (prioritySize > 0)){
            this->currentVirtualLinkIDKey = -1;
        }else{
            this->currentVirtualLinkIDKey = linkID;
        }
    }else{
        this->currentVirtualLinkIDKey = linkID;
    }
    EV_DETAIL << "[TschNeighbor] Selected Virtual Link ID is: " << this->currentVirtualLinkIDKey << endl;
}
void TschNeighbor::setMethod(std::string type){
    if(type == "First"){
        this->method = First;
        EV_DETAIL << "Selection method: First" << endl;
    }else if(type == "Longest"){
        this->method = Longest;
        EV_DETAIL << "Selection method: Longest" << endl;
    }
    // Additional selection methods can be added here
    else{
        EV_DETAIL << "No appropriate method is selected, thus the default method is selected" << endl;
        EV_DETAIL << "Selection method: First" << endl;
        this->method = First;
    }
}

bool TschNeighbor::isDedicated(){
    return this->dedicated;
}

int TschNeighbor::getQueueLength(){
    return this->queueLength;
}

void TschNeighbor::startTschCSMA(){
    this->getCurrentTschCSMA()->startTschCSMA();
}

bool TschNeighbor::getCurrentTschCSMAStatus(){
    return this->getCurrentTschCSMA()->getTschCSMAStatus();
}

void TschNeighbor::updateAllBackoffWindows(inet::MacAddress destAddress, TschSlotframe* sf){
    bool isBroadcast = destAddress.isBroadcast();
    for(auto itr = this->backoffTable.begin(); itr != this->backoffTable.end(); ++itr){
        if (itr->second->getRandomNumber() != 0) {
            bool hasTxLink = sf->hasLink(itr->first);
            if ((!hasTxLink && isBroadcast)
                    || (hasTxLink && (destAddress == itr->first))) {
                itr->second->decrementRandomNumber();
            }
        }
    }
}

std::map<inet::MacAddress, std::map<int, std::list<inet::Packet *>*>*>* TschNeighbor::getMacToQueueMap(){
    return &this->macToQueueMap;
}

std::map<inet::MacAddress,TschCSMA*>* TschNeighbor::getBackoffTable(){
    return &this->backoffTable;
}

std::list<inet::Packet *>* TschNeighbor::createQueue(){
    return new std::list<inet::Packet *>;
}



std::map<int, std::list<inet::Packet *>*>* TschNeighbor::createVirtualQueue(){
    return new std::map<int, std::list<inet::Packet *>*>;
}

void TschNeighbor::printQueue(){
    for(auto outer = this->macToQueueMap.begin(); outer != this->macToQueueMap.end(); ++outer){
        EV_DETAIL << "The MacAddress: " << outer->first << " queue:" << endl;
        for(auto inner = outer->second->begin(); inner != outer->second->end(); ++inner){
            EV_DETAIL << "virtualLinkID  " << inner->first << " has " << inner->second->size() << " packets" << endl;
        }
    }
    EV_DETAIL << ".........................." << endl;
    EV_DETAIL << ".........................." << endl;
}
void TschNeighbor::clearQueue(){
    for (auto itrOuter = this->macToQueueMap.begin();
            itrOuter != this->macToQueueMap.end(); ++itrOuter) {
        for (auto itrInner = itrOuter->second->begin();
                itrInner != itrOuter->second->end(); ++itrInner) {
            for (auto packet = itrInner->second->begin();
                    packet != itrInner->second->end(); ++packet) {
                delete *packet;
            }

        }
        //itrOuter->second->clear();
    }
    for(auto itr = this->backoffTable.begin(); itr != this->backoffTable.end(); ++itr){
        delete itr->second;
    }
}
}
