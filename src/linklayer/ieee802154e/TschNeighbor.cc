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

namespace tsch{

Define_Module(TschNeighbor);

TschNeighbor::~TschNeighbor() {
    for (int i = 0; i < (int)(this->Neighbor.size()); i++) {
        for(auto & packet: this->Neighbor[i]){
            delete packet;
        }
        this->Neighbor[i].clear();
    }
    delete this->backoff;
}

void TschNeighbor::initialize(int stage){
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL){
        hasPriorityQueue = par("hasPriorityQueue");
        this->addr.setBroadcast();
        this->selectedQueue.setBroadcast();
        this->Neighbor.push_back(this->normalQueue);
        if(hasPriorityQueue == true){
            this->Neighbor.push_back(this->priorityQueue);
        }
        this->macAddressTable.push_back(this->addr);
        this->backoff = new TschCSMA(par("macMinBE"),par("macMaxBE"));
        this->backoffTable.push_back(new TschCSMA(par("macMinBE"),par("macMaxBE")));
        this->resetMacAddress();
        this->dedicated = false;
        setMethod(par("method"));
        this->queueLength = par("queueLength");
    }

}

void TschNeighbor::handleMessage(cMessage *msg) {
    throw cRuntimeError("This module doesn't process messages");
}

bool TschNeighbor::add2Queue(Packet *packet, bool priority,
        MacAddress macAddr) {
    bool added = false;
    int position = this->checkAddressTable(macAddr);
    if (position == -1) { // i.e a new mac address
        // add a normal priority queue of a new neighbor
        this->Neighbor.push_back(this->normalQueue);
        if (!this->hasPriorityQueue) {
            // add packet to the end of the normal priority queue of the new neighbor (i.e. last vector entry)
            this->Neighbor.back().push_back(packet);
        } else {
            if (!priority) {
                // add packet to the end of the normal priority queue of the new neighbor (i.e. last vector entry)
                this->Neighbor.back().push_back(packet);
            }
            // add a high priority queue of a new neighbor
            this->Neighbor.push_back(this->priorityQueue);
            if (priority) {
                // add packet to the end of the high priority queue of the new neighbor (i.e. last vector entry)
                this->Neighbor.back().push_back(packet);
            }
        }
        this->macAddressTable.push_back(macAddr);
        this->backoff = new TschCSMA(par("macMinBE"),par("macMaxBE"));
        this->backoffTable.push_back(this->backoff);
        added = true;
    } else { // i.e a known mac address
        if (!this->hasPriorityQueue) {
            if((int) this->Neighbor[position].size() < this->queueLength){
                // add packet to the normal priority queue of the new neighbor at entry 'position'
                this->Neighbor[position].push_back(packet);
                added = true;
            }
        } else {
            if (!priority) {
                // with priority queues enabled, each neighbor has two queues in the vector, hence a two times position offset
                if ((int) this->Neighbor[position * 2].size() < this->queueLength) {
                    // add packet to the normal priority queue of the known neighbor at entry 'position*2'
                    this->Neighbor[position * 2].push_back(packet);
                    added = true;
                }

            } else {
                if ((int) this->Neighbor[(position * 2) + 1].size() < this->queueLength) {
                    // add packet to the high priority queue of the known neighbor at entry 'position*2 + 1'
                    this->Neighbor[(position * 2) + 1].push_back(packet);
                    added = true;
                }
            }
        }

    }
    return added;
}

int TschNeighbor::checkQueueSizeAt(MacAddress macAddress) {
    int queuePosition = this->checkAddressTable(macAddress);
    if( queuePosition == -1 ){
        return 0;
    }
    if(this->hasPriorityQueue){
        return this->Neighbor[queuePosition * 2].size() + this->Neighbor[queuePosition * 2 + 1].size();
    } else {
        return this->Neighbor[queuePosition].size();
    }
}

int TschNeighbor::checkNormalQueueSizeAt(int i) {
    if(this->hasPriorityQueue){
        return this->Neighbor[i * 2].size();
    }
    return this->Neighbor[i].size();
}

int TschNeighbor::checkPriorityQueueSizeAt(int i) {
    return this->Neighbor[i * 2 + 1].size();
}

int TschNeighbor::getCurrentNeighborQueueSize(){
    return this->Neighbor[this->currentQueuePosition()].size();
}

inet::Packet* TschNeighbor::getCurrentNeighborQueueFirstPacket(){
    return this->getNeighbor()[this->currentQueuePosition()].front();
}
void TschNeighbor::removeFirstPacketFromQueue(){
    this->Neighbor[this->currentQueuePosition()].pop_front();
}

void TschNeighbor::failedTX(){
    this->getCurrentTschCSMA()->failedTX(this->isDedicated());
}

void TschNeighbor::clear() {
    delete this->backoff;
    for (int i = 0; i < (int)(this->Neighbor.size()); i++) {
        for (auto & packet: this->Neighbor[i]) {
            delete packet;
        }
        this->Neighbor[i].clear();
    }
    for (auto & backoffs:  this->backoffTable){
        delete backoffs;
    }
}

void TschNeighbor::setSelectedQueue(MacAddress addr) {
    this->selectedQueue = addr;
}

inet::MacAddress& TschNeighbor::getSelectedQueue() {
    return this->selectedQueue;
}

int TschNeighbor::checkTotalQueueSize() {
    int total = 0;
    for (int i = 0; this->Neighbor.size(); i++) {
        total += this->Neighbor[i].size();
    }
    return total;

}

std::vector<std::list<inet::Packet *>>& TschNeighbor::getNeighbor() {
    return this->Neighbor;
}

std::string TschNeighbor::getMacAddressAtTable(int i) {
    return this->macAddressTable[i].str();
}

int TschNeighbor::checkAddressTable(MacAddress macAddress) {
    for (int i = 0; i < (int)(this->macAddressTable.size()); i++) {
        if (this->macAddressTable[i] == macAddress) {
            return i;
        }
    }
    return -1;
}

void TschNeighbor::setPriorityQueue(bool priority) {
    this->hasPriorityQueue = priority;
}

bool TschNeighbor::isPriorityQueue() {
    return this->hasPriorityQueue;
}
int TschNeighbor::getMacAddressTableSize(){
    return this->macAddressTable.size();
}

TschCSMA* TschNeighbor::getbackoffTableAt(int i){
    return this->backoffTable[i];
}

TschCSMA* TschNeighbor::getCurrentTschCSMA(){
    return this->backoffTable[this->currentQueuePosition()];
}

void TschNeighbor::terminateCurrentTschCSMA(){
    this->getCurrentTschCSMA()->terminate();
}


int TschNeighbor::currentQueuePosition() {
    return this->checkAddressTable(this->getSelectedQueue());
}

void TschNeighbor::resetMacAddress() {
    this->addr.setAddress(inet::MacAddress::UNSPECIFIED_ADDRESS.str().c_str());
}
void TschNeighbor::setDedicated(bool value){
    this->dedicated = value;
}

bool TschNeighbor::checkAndselectQueue(std::vector<inet::MacAddress> tempMacDedicated){
    bool found = false;
    switch (this->method) {
    // Iterate over all neighbors and select the first that matches(non-shared link, not in backoff, packet is not empty)
    case 1: {
        for (int i = 0; i < (int) this->macAddressTable.size(); i++) {
            bool match = false;
            for (int j = 0; j < (int) tempMacDedicated.size(); j++) {
                if (this->macAddressTable[i] == tempMacDedicated[j]) {
                    match = true;
                    break;
                }
            }
            if (match) {
                if (this->getbackoffTableAt(i)->getBE() == 0
                        && this->Neighbor[i].size() > 0) {
                    this->setSelectedQueue(this->macAddressTable[i]);
                    found = true;
                    break;
                }
            }
        }
        break;
    }
    case 2: {
        int temp_longest = -1;
        int temp_position = -1;
        for (int i = 0; i < (int) this->macAddressTable.size(); i++) {
            bool match = false;
            for (int j = 0; j < (int) tempMacDedicated.size(); j++) {
                if (this->macAddressTable[i] == tempMacDedicated[j]) {
                    match = true;
                    break;
                }
            }
            if (match) {
                if (this->getbackoffTableAt(i)->getBE() == 0
                        && this->Neighbor[i].size() > 0) {
                    if ((int)this->Neighbor[i].size() > temp_longest) {
                        temp_longest = this->Neighbor[i].size();
                        temp_position = i;
                        found = true;
                    }
                }
            }
            this->setSelectedQueue(tempMacDedicated[temp_position]);
            break;
        }
        break;
    }
    default: {
        break;
    }

    }
    return found;
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

}
