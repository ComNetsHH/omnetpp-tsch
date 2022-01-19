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
#include "TschVirtualLink.h"
#include "inet/common/INETUtils.h"
#include <iostream>
#include <algorithm>
#include <list>
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
        macMaxBe = par("macMaxBe").intValue();
        macMinBe = par("macMinBe").intValue();
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
    if(!this->enablePriorityQueue && (virtualLinkID == -1))
        virtualLinkID = 0;

    bool added = false;
    auto macToQueueEntry = this->macToQueueMap.find(macAddr);

    if (macToQueueEntry != macToQueueMap.end())
        EV_DETAIL << "[TschNeighbor] The given Macaddress: " << macAddr << " is already a Neighbor." << endl;
    else
    {
        EV_DETAIL << "[TschNeighbor] The given Macaddress: " << macAddr << " is not found. New entry added in Neighbor." << endl;
        this->macToQueueMap.insert(std::make_pair(macAddr, createVirtualQueue()));
    }

    //this->macToQueueMap.insert(std::make_pair(macAddr, createVirtualQueue()));
    // This if clause might be useless
    macToQueueEntry = this->macToQueueMap.find(macAddr);
    if (macToQueueEntry != macToQueueMap.end()) {
        // Insert()checks redundancy
        macToQueueEntry->second->insert(std::make_pair(-2, createQueue()));
        macToQueueEntry->second->insert(std::make_pair(-1, createQueue()));
        macToQueueEntry->second->insert(std::make_pair(0, createQueue()));
        macToQueueEntry->second->insert(std::make_pair(1, createQueue()));
        macToQueueEntry->second->insert(std::make_pair(virtualLinkID, createQueue()));
        auto searchVirtualQueue = macToQueueEntry->second->find(virtualLinkID);
        auto virtualQueueSize = (int)(searchVirtualQueue->second->size());
        EV_DETAIL << "[TschNeighbor] The current queue for this Neighbor is: " << virtualQueueSize << endl;

        if (virtualQueueSize < this->queueLength) {
            searchVirtualQueue->second->push_back(packet);
            EV_DETAIL << "[TschNeighbor] Packet is added to the queue." << endl;
            added = true;
        }
        else
            EV_DETAIL << "[TschNeighbor] The queue of this neighbor is full." << endl;
    }
    if (added)
        backoffTable.insert(std::make_pair(macAddr, new TschCSMA(macMinBe, macMaxBe, this->getRNG(0))));

    return added;
}

int TschNeighbor::getTotalQueueSizeAt(MacAddress macAddress) {
    auto macToQueueEntry = this->macToQueueMap.find(macAddress);
    if (macToQueueEntry != this->macToQueueMap.end()) {
        int queueSize= 0;
        // iterate over all virtual link IDs, i.e. priority levels
        for (auto itr = macToQueueEntry->second->begin(); itr != macToQueueEntry->second->end(); ++itr)
            queueSize += (int)itr->second->size();

        return queueSize;
    } else {
        EV_DETAIL << "[TschNeighbor] This Neighbor does not exist." << endl;
        return 0;
    }
}

int TschNeighbor::getVirtualQueueSizeAt(MacAddress macAddress, int virtualLinkId) {
    auto entry = this->macToQueueMap.find(macAddress);
    if (entry == this->macToQueueMap.end())
        return 0;

    auto virtualQueue = entry->second->find(virtualLinkId);
    if (virtualQueue == entry->second->end())
        return 0;

    return (int) virtualQueue->second->size();
}

void TschNeighbor::setVirtualQueue(MacAddress macAddr, int linkId) {
    this->currentNeighborKey = macAddr;
    this->currentVirtualLinkIDKey = linkId;
    EV_DETAIL << "[TschNeighbor] Selected neighbor " << macAddr
        << " and virtual link ID " << this->currentVirtualLinkIDKey << endl;
}

int TschNeighbor::getCurrentNeighborQueueSize(){
    int queueSize = 0;
    auto search = this->macToQueueMap.find(this->currentNeighborKey);
    if (search != this->macToQueueMap.end())
    {
        for(auto itr = search->second->begin(); itr != search->second->end(); ++itr)
            queueSize += (int)itr->second->size();

        EV_DETAIL << "[TschNeighbor] The queue size for the current neighbor is: " << queueSize << endl;
        return queueSize;
    } else {
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

void TschNeighbor::flushQueue(MacAddress neighbor, int vlinkId) {
    auto neighborQueueInfo = this->macToQueueMap.find(neighbor);
    if (neighborQueueInfo == this->macToQueueMap.end())
        return;

    auto virtualQueue = neighborQueueInfo->second->find(vlinkId);
    if (virtualQueue == neighborQueueInfo->second->end())
        return;

    virtualQueue->second->clear();

    EV_DETAIL << "Flushed the queue with virtual link ID " << vlinkId << " for " << neighbor << endl;
}

void TschNeighbor::flush6pQueue(MacAddress neighbor) {
    EV_DETAIL << "Flushing queue with " << neighbor << endl;
    auto neighborQueueInfo = this->macToQueueMap.find(neighbor);
    if (neighborQueueInfo == this->macToQueueMap.end())
        return;

    auto virtualQueue = neighborQueueInfo->second->find(LINK_PRIO_CONTROL);
    if (virtualQueue == neighborQueueInfo->second->end())
        return;

    EV_DETAIL << "control queue before: " << printPacketQueue(virtualQueue->second) << endl;

    virtualQueue->second->remove_if(
            [](Packet *pkt) {
                std::string pktName(pkt->getFullName());
                return pktName.find("6top") != std::string::npos;
            });

    EV_DETAIL << "control queue after:  " << printPacketQueue(virtualQueue->second) << endl;
}

TschCSMA* TschNeighbor::getCurrentTschCSMA(){
    return this->backoffTable.find(currentNeighborKey)->second;
}

TschCSMA* TschNeighbor::getTschCsmaWith(MacAddress neighborAddr) {
    return this->backoffTable.find(neighborAddr)->second;
}

void TschNeighbor::terminateTschCsmaWith(MacAddress neighborAddr) {
    EV_DETAIL << "Terminating CSMA with " << neighborAddr << endl;

    auto nbrCsma = getTschCsmaWith(neighborAddr);
    if (nbrCsma)
        nbrCsma->terminate();
}

void TschNeighbor::failedTX(){
    this->getCurrentTschCSMA()->failedTX(this->isDedicated());
}

void TschNeighbor::terminateCurrentTschCSMA(){
    EV_DETAIL << "Terminating CSMA with " << currentNeighborKey << endl;
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

bool TschNeighbor::isDedicated() {
    return this->dedicated;
}

int TschNeighbor::getQueueLength() {
    return this->queueLength;
}

void TschNeighbor::startTschCSMA() {
    this->getCurrentTschCSMA()->startTschCSMA();
}

bool TschNeighbor::getCurrentTschCSMAStatus() {
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

void TschNeighbor::printQueue() {
    for (auto outer = this->macToQueueMap.begin(); outer != this->macToQueueMap.end(); ++outer)
    {

        if (getTotalQueueSizeAt(outer->first) == 0)
            continue;

        EV_DETAIL << "The MacAddress: " << outer->first << " queue:" << endl;

        for (auto inner = outer->second->begin(); inner != outer->second->end(); ++inner) {

            if (inner->second->size())
                EV_DETAIL << "virtualLinkID " << inner->first << " has " << inner->second->size() << " packets" << endl;
        }
    }
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
