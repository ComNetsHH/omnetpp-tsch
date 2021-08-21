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

#ifndef LINKLAYER_IEEE802154E_TSCHCSMA_H_
#define LINKLAYER_IEEE802154E_TSCHCSMA_H_

#include <omnetpp.h>
#include "inet/common/INETDefs.h"

using namespace omnetpp;
using namespace inet;
namespace tsch {
/**
 * Class to start the Tsch CSMA.
 */
class TschCSMA {
public:
    /**
     * A constructor which sets the NB, BE and randomNumber to -1.
     */
    TschCSMA();
    /**
     * A constructor which sets the NB, BE and randomNumber to -1 and the private macMinBE and macMaxBE to the two arguments.
     * @param minBE an integer which represents the minimum backoff exponent
     * @param maxBE an integer which represents the maximum backoff exponent
     */
    TschCSMA(int, int, cRNG *);
    /**
     * Default contructor.
     */
    virtual ~TschCSMA();
    /**
     * A public member function taking one int argument to set the number of Backoffs(NB).
     * @param an integer to set NB
     */
    void setNB(int);
    /**
     * A public member function to get the private NB variable.
     * @return number of backoffs
     */
    int getNB();
    /**
     * A public member function taking one int argument to set Backoff exponent(BE).
     * @param an integer to set BE
     */
    void setBE(int);
    /**
     * A public member function to get the private BE variable.
     * @return current backoff exponent
     */
    int getBE();
    /**
     * A public member function taking one int argument to set the minimum Backoff exponent(macMinBe).
     * @param an integer to set macMinBE
     */
    void setMacMinBE(int);
    /**
     * A public member function to get the private macMinBE variable.
     * @return minimum backoff exponent
     */
    int getMacMinBE();
    /**
     * A public member function taking one int argument to set the maximum Backoff exponent(macMaxBe).
     * @param an integer to set macMaxBE
     */
    void setMacMaxBE(int);
    /**
     * A public member function to get the private macMinBE variable.
     * @return maximum backoff exponent
     */
    int getMacMaxBE();
    /**
     * A public member function to get the private randomNumber variable.
     * @return randomNumber variable
     */
    int getRandomNumber();
    /**
     * A public member function generating a random number using the function intuniform() from omnetpp, setting and returning the randomNumber to it.
     * The generated random number depends on BE and NB
     * @return The generated random number
     */
    int generateRandomNumber();
    /**
     * A public member function taking in one argument which represents the minimum BE to start the Tsch CSMA.
     * Sets the NB to 0, BE to the argument and generates and sets the randomNumber
     * @see generateRandomNumber()
     */
    void startTschCSMA();
    /**
     * A public member function to check if the CSMA is still in the backoff and returns the state.
     * If the randomNumber is 0 the function return true
     * If the randomNumber is not 0 it will reduce the randomnumber by 1 and return false
     * @return state of the backoff
     */
    bool checkBackoff();
    /**
     * A public member function taking one int argument which represents the maximumg backoff exponent increases the NB and BE.
     * NB is increased by 1 and the BE takes the minimum of either an incremented BE or the argument
     * @param an integer maximum BE to increase the BE
     */
    void failedTX(bool isDedicated);
    /**
     * A public member function to reset the private values to -1.
     */
    void terminate();
    /**
     * A public member function to increment the number of backoffs.
     * This function is called if the TschCSMA itself has not started but the transmission failed due to busy channel
     */
    void increaseNumberOfBackoff();
    /**
     * A public member function to return the status of the TschCSMA algorithm
     * @return state of the TschCSMA algorithm
     */
    bool getTschCSMAStatus();

    void setRng(cRNG* rng);

    std::string str();
    /**
     * A public member function to decrement the current backoff state by 1
     *
     */
    void decrementRandomNumber();
private:
    /**
     * private variable
     * Represents the number of backoffs
     */
    int NB;
    /**
     * private variable
     * Represents the backoff exponent
     */
    int BE;
    /**
     * private variable
     * Represents the current backoff state of the CSMA
     */
    int randomNumber;
    /**
     * Shows the minimum backoff exponent
     */
    int macMinBE;
    /**
     * Shows the maximum backoff exponent
     */
    int macMaxBE;
    /**
     * private variable
     * Shows if the TschCSMA algorithm has already started
     */
    bool hasStarted;

    /**
     * random number generator to use
     */
    cRNG *rng;
};
}
#endif /* LINKLAYER_IEEE802154E_TSCHCSMA_H_ */
