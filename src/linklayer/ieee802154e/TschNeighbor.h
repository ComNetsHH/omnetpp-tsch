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

#ifndef LINKLAYER_IEEE802154E_TSCHNEIGHBOR_H_
#define LINKLAYER_IEEE802154E_TSCHNEIGHBOR_H_

#include <omnetpp.h>
#include "inet/linklayer/common/MacAddress.h"
#include "inet/common/packet/Packet.h"
#include "TschCSMA.h"
#include <vector>

using namespace inet;
namespace tsch{
/**
 * Class to organize a normal priority queue and a high priority queue each for all individual neighbors
 */
class TschNeighbor : public cSimpleModule, protected cListener
{
    private:
        /**
         * Alias for std::list<inet::Packet *>
         */
        typedef std::list<inet::Packet *> Queue;
        /**
         * private variable
         * A normal priority list filled with inet::Packet pointers.
         */
        Queue normalQueue;
        /**
         * private variable
         * A high priority list filled with inet::Packet pointers.
         */
        Queue priorityQueue;
        /**
         * private variable
         * A vector of queue lists for each of the neighbors of the node
         */
        std::vector<std::list<inet::Packet *>> Neighbor;
        /**
         * private variable
         * A vector of MAC addresses
         */
        std::vector<inet::MacAddress> macAddressTable;
        /**
         * private variable
         * A vector of TschCSMA pointer elements to start the Tsch CSMA
         */
        std::vector<TschCSMA*> backoffTable;
        /**
         * private variable
         * A inet::MacAddress object used to simplify some functions
         */
        inet::MacAddress addr;
        /**
         * private variable
         * A inet::MacAddress object which represents the currently used neighbor
         */
        inet::MacAddress selectedQueue;
        /**
         * private variable
         * A TschCSMA pointer used to simplify some functions
         */
        TschCSMA* backoff;
        /**
         * private variable
         * Shows if a priority queue is used
         */
        bool hasPriorityQueue;
        /**
         * private variable
         * Shows if the current transmission is transmitted using a dedicated or shared slot
         */
        bool dedicated;
        // TODO delete
        bool broadcast;
        bool timesource; // TODO unused
        /**
         * private variable
         * Using the NED file the selection method in case a shared broadcast slot occurs and that queue is empty
         * Does not Work yet
         */
        // TODO Parser for the string input
        enum selectionMethod: int {
                First = 1,
                Longest = 2,
            };
        /**
         * private variable
         * Currently used integer variable to determine the selection method
         */
        selectionMethod method;

        // TODO: Integrate queueLength into the class
        /**
         * private variable
         * Shows the maximum queue length
         */
        int queueLength;
    public:
        /**
         * Default constructor
         * This constructor is not used since this class is derived from cSimpleModule.
         * The initialization of all variables happens in initialize()
         *@see virtual void initialize(int stage) override;
         */
        TschNeighbor(){};
        /**
         * Default destructor
         * Deletes all the packets in the queues and the TschCSMA backoff
         */
        virtual ~TschNeighbor();
        /**
         * Adds a packet a the designated queue dependent on the bool element priority queue length and the MacAddress and returns if the packet is successfully added to the queue
         * @param packet a pointer which points to a inet::packet
         * @param priority a bool which shows if this packet is designated for the priority or normal queue
         * @param macAddr a inet::MacAddress which determines to which queue the packet is pushed to
         * @return Packet is successfully added or not
         */
        bool add2Queue(inet::Packet *,bool, MacAddress);
        /**
         * A public member function taking one inet::MacAddress object to get the position of the object with the same MacAddress in the Neighbor and returns it.
         * @param macAddress a inet::MacAddress object to search for the position in the Neighbor
         * @return Position in the Neighbor or -1 if not found
         *
         */
        int checkAddressTable(MacAddress);
        /**
         * A public member function to reset the private addr member variable to UNSPECIFIC_ADDRESS
         */
        void resetMacAddress();
        /**
         * Determines the total queue size of a specific neighbor identified by its MAC address
         * @param macaddress a inet::MacAddress argument to search for that MacAddress in the queue
         * @return Summed-up size of the normal and priority queue with the input MacAddress or 0 if the queue with that MacAddress is not found
         * @see int checkAddressTable(MacAddress)
         */
        int checkQueueSizeAt(MacAddress);
        /**
         * A public member function taking the position of a MacAddress in the neighbor queue to get the queue size with the same MacAddress for the normal queue
         * @param i the position of a MacAddress in the neighbor queue
         * @return Size of the queue with the input MacAddress or 0 if the queue with that MacAddress is not found
         * @see int checkAddressTable(MacAddress)
         */
        int checkNormalQueueSizeAt(int);
        /**
         * A public member function taking the position of a MacAddress in the neighbor queue to get the queue size with the same MacAddress for the priority queue
         * @param i the position of a MacAddress in the neighbor queue
         * @return Size of the queue with the input MacAddress or 0 if the queue with that MacAddress is not found
         * @see int checkAddressTable(MacAddress)
         */
        int checkPriorityQueueSizeAt(int);
        /**
         * A public member function to return the size of the currently used neighbor queue
         * @return Size of the currently used queue
         */
        int getCurrentNeighborQueueSize();
        /**
         * A public member function to return the first packet of the currently used neighbor queue
         * @return Pointer to the first packet of the currently used queue
         */
        inet::Packet* getCurrentNeighborQueueFirstPacket();
        // TODO check if the function is used
        void clear();
        /**
         * A public member function taking one inet::MacAddress object to set the currently used queue
         * @param addr a inet::MacAddress argument to the set the selected queue
         */
        void setSelectedQueue(MacAddress);
        /**
         * A public member function to return the address of the selectedQueue
         * @return MacAddress of the private member variable selectedQueue
         */
        inet::MacAddress& getSelectedQueue();
        /**
         * A public member function to get the total number of all packets in all queues
         * @return Total packets
         */
        int checkTotalQueueSize();
        /**
         * A public member function which returns a integer value which represents the position of the selected Queue in the Neighbor vector
         * @return position of the selected queue in the Neighbor vector
         */
        int currentQueuePosition();
        /**
         * A public member function to get the private Neighbor vector.
         * @return reference to the private Neighbor vector
         */
        std::vector<Queue>& getNeighbor();
        /**
         * A public member function to set if there is a priority queue
         * @param priority a bool to set the priority queue
         */
        void setPriorityQueue(bool);
        /**
         * A public member function to check if there is a priority queue
         * @return True if priorityQueue is set to true or false if otherwise
         */
        bool isPriorityQueue();
        /**
         * A public member function to get the total number of neighbors of the node
         * @return Number of neigbors
         */
        int getMacAddressTableSize();
        /**
         * A public member function to get the MacAddress in string form in at the input argument position.
         *@param  i a integer which represents the position
         *@return MacAddress in string form
         */
        std::string getMacAddressAtTable(int);
        /**
         * A public member function taking one integer argument to get the TschCSMA pointer at the position of the input argument
         * @ i a integer which represents the position
         * @return pointer to the TschCSMA
         */
        TschCSMA* getbackoffTableAt(int);
        /**
         * A public member function returning a pointer to the TschCSMA of the currently used neighbor
         * @return pointer to the TschCSMA
         */
        TschCSMA* getCurrentTschCSMA();
        /**
         * A public member function checking if the slot is dedicated
         * @return  State of the slot
         */
        bool isDedicated();
        /**
         * A public member function to set the private member variable dedicated.
         * @param value a bool to set the dedicated variable
         */
        void setDedicated(bool);
        /**
         * A public member function taking one std::vector<inet::MacAddress > selecting an alternative and appropriate queue and returning a true in case one is found.
         * @param tempMacDedicated a std::vector<inet::MacAddress > which represents all neighbors which met the criteria of having no dedicated link, not in backoff and the queue is not empty
         * @return True if one is found otherwise false
         */
        bool checkAndselectQueue(std::vector<inet::MacAddress >);
        /**
         * A public member function taking one std::string to select the selection method for the checkAndselectQueue function.
         * @param type a std::string which represents the selection method for the checkAndselectQueue (Set in the TschNeighbor.ned)
         */
        void setMethod(std::string);
        /**
         * A public member function to get the private member queueLength
         * @param queue length
         */
        int getQueueLength();
        /**
         * A public member function to call the terminate function of the TschCSMA object
         *
         */
        void terminateCurrentTschCSMA();
        /**
         * A public member function to remove the first packet from the currently used queue
         */
        void removeFirstPacketFromQueue();
        /**
         * A public member function to signal a failed transmission to the TschCSMA object
         */
        void failedTX();
        /**
         * A public member function to signal the TschCSMA object of the current neighbor to start the algorithm
         */
        void startTschCSMA();
        /**
         * A public member function to return the TschCSMA status of the current neighbor
         * @return True if TschCSMA has started
         */
        bool getCurrentTschCSMAStatus();


    protected:
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void handleMessage(cMessage *) override;
};
}
#endif /* LINKLAYER_IEEE802154E_TSCHNEIGHBOR_H_ */
