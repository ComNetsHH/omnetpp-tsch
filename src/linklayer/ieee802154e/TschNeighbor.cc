/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright (C) 2021  Institute of Communication Networks (ComNets),
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
#include <algorithm>
#include <array>

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
        this->W_npq = 4;
        this->W_nq = 1;
        this->C_npq = this->W_npq;
        this->C_nq = this->W_nq;
    }else if(stage == 5){
        hostNode = getModuleByPath("^.^.^.");
    }

}

void TschNeighbor::handleMessage(cMessage *msg) {
    throw cRuntimeError("This module doesn't process messages");
}
// New virtual queue
bool TschNeighbor::add2Queue(Packet *packet,MacAddress macAddr, int virtualLinkID) {
    // Priority queue disabled but virtualLinkID modified??
    // Guess: for test purposes, just to toggle the enablePriorityQueue
    // If priority disabled, override the virtualLinkID other than 0 to 0
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
        // Insert()checks redundancy
        searchMacAddress->second->insert(std::make_pair(-2, createQueue()));
        searchMacAddress->second->insert(std::make_pair(-1, createQueue()));
        searchMacAddress->second->insert(std::make_pair(0, createQueue()));
        searchMacAddress->second->insert(std::make_pair(1, createQueue()));
        searchMacAddress->second->insert(std::make_pair(virtualLinkID, createQueue()));
        searchMacAddress->second->insert(std::make_pair(99, createQueue()));
        auto searchVirtualQueue = searchMacAddress->second->find(virtualLinkID);
        auto virtualQueueSize = (int)(searchVirtualQueue->second->size());
        EV_DETAIL << "[TschNeighbor] The current queue for this Neighbor is: " << virtualQueueSize << endl;
        if(virtualQueueSize < this->queueLength){
            searchVirtualQueue->second->push_back(packet);
            EV_DETAIL << "[TschNeighbor] Packet is added to the queue." << endl;
            added = true;
        }else{
            EV_DETAIL << "[TschNeighbor] The queue of this Neighbor is full." << endl;
        }
    }
    if(added){
        backoffTable.insert(std::make_pair(macAddr, new TschCSMA(par("macMinBE"), par("macMaxBE"), this->getRNG(0))));
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

// input -1, get size(-1)
// input 0, get size(-1) + size(0) + size(1)
// input x\{-1,0}, get size(x)
// So weird
int TschNeighbor::checkVirtualQueueSizeAt(MacAddress macAddress,int virtualLinkID){
    auto search = this->macToQueueMap.find(macAddress);
    if(search != this->macToQueueMap.end()){
        auto virtualQueue = search->second;
        if(virtualLinkID == -1 || virtualLinkID == 0){
            if(virtualQueue->find(-1) != virtualQueue->end() || virtualQueue->find(0) != virtualQueue->end() || virtualQueue->find(1) != virtualQueue->end()){
                if (virtualLinkID == -1) {
                    int prioritySize = 0;
                    auto priorityQueue = virtualQueue->find(-1);
                    if (priorityQueue != virtualQueue->end()) {
                        prioritySize = priorityQueue->second->size();
                    }
                    return prioritySize;
                } else{
                    int controlQueueSize = 0;
                    int strictPrioritySize = 0;
                    int normalPrioritySize = 0;
                    int normalSize = 0;
                    auto controlQueue = virtualQueue->find(-2);
                    if (controlQueue != virtualQueue->end()) {
                        controlQueueSize = controlQueue->second->size();
                    }
                    auto strictPriorityQueue = virtualQueue->find(-1);
                    if (strictPriorityQueue != virtualQueue->end()) {
                        strictPrioritySize = strictPriorityQueue->second->size();
                    }
                    auto normalPriorityQueue = virtualQueue->find(1);
                    if (normalPriorityQueue != virtualQueue->end()) {
                        normalPrioritySize = normalPriorityQueue->second->size();
                    }
                    auto normalQueue = virtualQueue->find(0);
                    if (normalQueue != virtualQueue->end()) {
                        normalSize = normalQueue->second->size();
                    }

                    int sum = strictPrioritySize + normalSize + normalPrioritySize + controlQueueSize;
//                    std::cout << "SP, NP, BE is repectively: " << strictPrioritySize << " " << normalPrioritySize << " " << normalSize << endl;

                    return sum;
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

int TschNeighbor::getCurrentVirtualLinkIDKey(){
    return this->currentVirtualLinkIDKey;
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

// Choose a neighbor
bool TschNeighbor::checkAndselectQueue(std::vector<inet::MacAddress> tempMacDedicated, TschSlotframe* sf) {

    auto it = macToQueueMap.end();

    // we check the queues in order (e.g. first priority, then normal)
    // if there is a neighbor with packets in the priority queue
    // and it fulfills all requirements, we give priority for that
    // otherwise we look into the next queue defined in the queues array
    static auto queues = std::array<int, 2>({{ -1, 0 }});

    for(const auto& queue : queues) {

        // pick the first neighbor that matches
        if (this->method == 1) {

            // here is where the magic happens: the lambda function combines multiple other helper functions
            it = std::find_if(macToQueueMap.begin(), macToQueueMap.end(), [queue, sf, this](decltype(macToQueueMap)::value_type entry) -> bool {
                        return _isSuitable(entry.first, sf) && _hasPacketsInQueue(entry, queue);
            });

        // pick the neighbor with the fullest queue
        } else if (this->method == 2) {

            // here is where the magic happens: the lambda function combines multiple other helper functions
            it =  std::max_element(macToQueueMap.begin(), macToQueueMap.end(), [queue, sf, this](decltype(macToQueueMap)::value_type a, decltype(macToQueueMap)::value_type b) -> bool {
                // if b is not suitable, it cannot be larger than a
                if (!_isSuitable(b.first, sf)) {
                    return false;
                }
                // if a is not suitable, it is smaller than b
                if (!_isSuitable(a.first, sf)) {
                    return true;
                }

                return _hasFullerQueue(a, b, queue);
            });

            // we found one, but it actually does not have packets or isn't suitable
            if (it != macToQueueMap.end() && !_hasPacketsInQueue(*it, queue)) {
                it = macToQueueMap.end();
            }

        } else {
            EV_ERROR << "TschNeighbor: Invalid selection method..." << endl;
            return false;
        }

        // we found one, use it and return
        if (it != macToQueueMap.end()) {
            this->setSelectedQueue(it->first, 0);
            return true;
        }
    }

    return false;
}

bool TschNeighbor::_isSuitable(inet::MacAddress mac, TschSlotframe* sf) {
    // only consider neighbors that are not in backoff & that do not (!) have tx-links
    return _isSuitableRelaxed(mac) && !sf->hasLink(mac);
}

bool TschNeighbor::_isSuitableRelaxed(inet::MacAddress mac) {
    // only consider non-broadcast neighbors
    if (mac != MacAddress::BROADCAST_ADDRESS
        // that are not in backoff
        && !(
              this->backoffTable.count(mac) > 0
              // be careful if the implementation of checkBackoff() changes...
              // would be nice to have sth like isInBackoff() instead
              && !this->backoffTable.at(mac)->checkBackoff()
            )
       ) {
        return true;
    }
    return false;
}

bool TschNeighbor::_hasPacketsInQueue(decltype(macToQueueMap)::value_type entry, int queue) {
      // queue exists & is not empty --> possible neighbor
      // count(..) on maps can only every return 1, as keys are unique. so count(..) is efficient here
      if (entry.second->count(queue) > 0 && entry.second->at(queue)->size() > 0) {
          return true;
      }
      return false;
}

bool TschNeighbor::_hasFullerQueue(decltype(macToQueueMap)::value_type a, decltype(macToQueueMap)::value_type b, int queue) {
    // if b does not have a matching queue, it cannot be larger than a
    if (b.second->count(queue) == 0) {
        return false;
    }
    // if a does not have a matching queue, it is smaller than b
    if (a.second->count(queue) == 0) {
        return true;
    }

    // compare actual queue sizes
    return a.second->at(queue)->size() < b.second->at(queue)->size();
}

// Purpose: schedule currentVirtualLinkKey
// TODO: The key function to realize PQ and WRR algorithm
void TschNeighbor::setSelectedQueue(MacAddress macAddr, int linkID) {
    this->currentNeighborKey = macAddr;
    // The linkID is by default 0 if cell virtual link info doesn't exist
    if(linkID == 0 ){
        int sixPsize = 0;
        int strictPrioritySize = 0;
        int normalPrioritySize = 0;
        int normalSize = 0;
        auto controlPacketQueue = this->macToQueueMap.find(this->currentNeighborKey)->second->find(-2);
        auto strictPriorityQueue = this->macToQueueMap.find(this->currentNeighborKey)->second->find(-1);
        auto normalPriorityQueue = this->macToQueueMap.find(this->currentNeighborKey)->second->find(1);
        auto normalQueue = this->macToQueueMap.find(this->currentNeighborKey)->second->find(0);
        if(controlPacketQueue != this->macToQueueMap.find(this->currentNeighborKey)->second->end()){
            sixPsize = (int)(controlPacketQueue->second->size());
        }
        if(strictPriorityQueue != this->macToQueueMap.find(this->currentNeighborKey)->second->end()){
            strictPrioritySize = (int)(strictPriorityQueue->second->size());
        }
        if(normalPriorityQueue != this->macToQueueMap.find(this->currentNeighborKey)->second->end()){
            normalPrioritySize = (int)(normalPriorityQueue->second->size());
        }
        if(normalQueue != this->macToQueueMap.find(this->currentNeighborKey)->second->end()){
                    normalSize = (int)(normalQueue->second->size());
        }
        // Strict priority
        if(this->macToQueueMap.find(this->currentNeighborKey)->second->count(-2) && (sixPsize > 0)){
            this->currentVirtualLinkIDKey = -2;
        }else if(this->macToQueueMap.find(this->currentNeighborKey)->second->count(-1) && (strictPrioritySize > 0)){
            this->currentVirtualLinkIDKey = -1;
        }else{
            // WRR W = 4
            if((this->C_npq > 0) && (normalPrioritySize > 0) && this->macToQueueMap.find(this->currentNeighborKey)->second->count(1)){
                this->currentVirtualLinkIDKey = 1;
                this->C_npq--;
                return;
            }
            // WRR W = 1
            if((this->C_nq > 0) && (normalSize > 0) && this->macToQueueMap.find(this->currentNeighborKey)->second->count(0)){
                this->currentVirtualLinkIDKey = 0;
                this->C_nq--;
                if(this->C_nq == 0){
                    this->C_npq = this->W_npq;
                    this->C_nq = this->W_nq;
                }
            }else{
                // If no packets in Normal queue while Normal Priority queue has packet,
                // Start a new round.
                // Because before this function is evoked, total queue size is assured to be > 0
                this->C_npq = this->W_npq;
                this->C_nq = this->W_nq;
                this->currentVirtualLinkIDKey = 1;
                this->C_npq--;


            }
        }
    }else{
        this->currentVirtualLinkIDKey = linkID;
    }
    EV_DETAIL << "[TschNeighbor] Selected neighbor " << macAddr.str() << " and Virtual Link ID " << this->currentVirtualLinkIDKey << endl;
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
    EV_DETAIL << ".........................." << endl;
    for(auto outer = this->macToQueueMap.begin(); outer != this->macToQueueMap.end(); ++outer){
        EV_DETAIL << "The MacAddress: " << outer->first << " queue:" << endl;
        for(auto inner = outer->second->begin(); inner != outer->second->end(); ++inner){
            EV_DETAIL << "virtualLinkID  " << inner->first << " has " << inner->second->size() << " packets" << endl;
        }
    }
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

void TschNeighbor::refreshDisplay() const {
//    int queueSize = 0;
//    for (auto itrOuter = this->macToQueueMap.begin(); itrOuter != this->macToQueueMap.end(); ++itrOuter) {
//        for(auto itrInner = itrOuter->second->begin(); itrInner != itrOuter->second->end(); ++itrInner) {
//                queueSize += (int)itrInner->second->size();
//        }
//    }    std::ostringstream out;
//    out << queueSize;
//    if(queueSize > 0){
//        hostNode->getDisplayString().setTagArg("t", 2, "#fc2803");
//    }else{
//        hostNode->getDisplayString().setTagArg("t", 2, "#035bfc");
//    }
//    hostNode->getDisplayString().setTagArg("t", 0, out.str().c_str());
}
}
