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

#include "TschCSMA.h"
#include <cmath>
#include <algorithm>

namespace tsch{

TschCSMA::TschCSMA(){
    this->NB = 0;
    this->BE = 0;
    this->randomNumber = 0;
    this->hasStarted = false;
}

TschCSMA::TschCSMA(int minBE, int maxBE){
    this->NB = 0;
    this->BE = 0;
    this->randomNumber = 0;
    this->macMinBE = minBE;
    this->macMaxBE = maxBE;
    this->hasStarted = false;
}

TschCSMA::~TschCSMA() {
}


void TschCSMA::startTschCSMA(){
    this->BE = this->macMinBE;
    this->generateRandomNumber();
    this->hasStarted = true;
    EV_DETAIL<< "Tsch CSMA has started" << endl;
}

int TschCSMA::generateRandomNumber(){

    this->randomNumber =intuniform(getEnvir()->getRNG(0), 0, ((int)std::pow(2.0, this->BE)-1));
    return this->randomNumber;
}

bool TschCSMA::checkBackoff(){
    if(this->randomNumber == 0){
        return true;
    }else{
        this->randomNumber = this->randomNumber - 1;
        return false;
    }
}
void TschCSMA::terminate(){
    this->BE = 0;
    this->NB = 0;
    this->randomNumber = 0;
    this->hasStarted = false;
}
void TschCSMA::failedTX(bool isDedicated){
    // Three cases:
    // 1 Dedicated->only the NB is increased by 1,
    // 2 TschCSMA has not started-> NB increased by 1,
    // 3 TschCSMA started->increase NB by 1, new BE and randomNumber

    if(isDedicated){
        this->NB++;
        EV_DETAIL << "The dedicated transmission failed due to busy channel during CCA" << endl;
        EV_DETAIL << "NB incremented by 1: NB= " << this->NB<< endl;
        EV_DETAIL << "The packet will be retransmitted in the next suitable tx link" << endl;
        EV_DETAIL << "Returning to Idle state" << endl;
    }else if(!this->hasStarted){
        this->NB++;
        EV_DETAIL << "The transmission failed due to busy channel during CCA" << endl;
        EV_DETAIL << "TschCSMA backoff has not started yet" << endl;
        EV_DETAIL << "NB incremented by 1: NB= " << this->NB<< endl;
        EV_DETAIL << "The packet will be retransmitted in the next suitable tx link" << endl;
        EV_DETAIL << "Returning to Idle state" << endl;

    }else{
        this->NB++;
        this->BE = std::min(this->BE+1,this->macMaxBE);
        this->generateRandomNumber();
        EV_DETAIL << "The transmission failed" << endl;
        EV_DETAIL << "TschCSMA has already started" << endl;
        EV_DETAIL << "NB incremented by 1: NB= " << this->NB<< endl;
        EV_DETAIL << "A new value has been selected for BE: BE= " << this->BE<< endl;
        EV_DETAIL << "A new value has been generated for the delay: Delay= " << this->randomNumber << endl;
        EV_DETAIL << "Returning to Idle state" << endl;
    }
}



void TschCSMA::setBE(int i){
    this->BE = i;
}

int TschCSMA::getBE(){
    return this->BE;
}

void TschCSMA::setNB(int i){
    this->NB = i;
}

int TschCSMA::getNB(){
    return this->NB;
}

void TschCSMA::setMacMinBE(int minBE) {
    this->macMinBE = minBE;
}

int TschCSMA::getMacMinBE() {
    return this->macMinBE;
}

void TschCSMA::setMacMaxBE(int maxBE) {
    this->macMaxBE = maxBE;
}


int TschCSMA::getMacMaxBE() {
    return this->macMaxBE;
}

bool TschCSMA::getTschCSMAStatus(){
    return this->hasStarted;
}
}
