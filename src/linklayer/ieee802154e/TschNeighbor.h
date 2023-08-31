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

#ifndef LINKLAYER_IEEE802154E_TSCHNEIGHBOR_H_
#define LINKLAYER_IEEE802154E_TSCHNEIGHBOR_H_

#include <omnetpp.h>
#include "inet/linklayer/common/MacAddress.h"
#include "inet/common/packet/Packet.h"
#include "TschCSMA.h"
#include "TschSlotframe.h"
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
        typedef std::list<inet::Packet *>* Queue;
        /**
         * private variable
         * A list filled with inet::Packet pointers.
         */
        Queue queue;
        /**
         * private variable
         * Maps queue to different virtuallinkID/Applications
         */
        std::map<int, Queue>* virtualQueue;
        /**
         * private variable
         * Maps Macaddress to different virtual queues
         */
        std::map<inet::MacAddress, std::map<int, Queue>*> macToQueueMap;

        /**
         * private variable
         * A container of TschCSMA pointer elements to start the Tsch CSMA
         */
        std::map<inet::MacAddress,TschCSMA*> backoffTable;
        /**
         * private variable
         * A inet::MacAddress object used to simplify some functions
         */
        inet::MacAddress addr;
        /**
         * private variable
         * The key  which represents the currently used Neighbor
         */
        inet::MacAddress currentNeighborKey;
        /**
         * private variable
         * The key  which represents the currently used Application
         */
        int currentVirtualLinkIDKey;
        /**
         * private variable
         * Shows if the current transmission is transmitted using a dedicated or shared slot
         */
        bool dedicated;
        /**
         * private variable
         * Decides if in the checkandselect function dedicated neighbors are included in the
         * selection in case no other suitable shared neighbor is found
         */
        bool enableSelectDedicated;
        /**
         * private variable
         * Decides if there is a priority queue
         */
        bool enablePriorityQueue;
        /**
         * private variable
         * Using the NED file the selection method in case a shared broadcast slot occurs and that queue is empty
         * Does not Work yet
         */
        enum selectionMethod: int {
                First = 1,
                Longest = 2,
            };
        /**
         * private variable
         * Currently used integer variable to determine the selection method
         */
        selectionMethod method;
        /**
         * private variable
         * Shows the maximum queue length
         */
        int queueLength;

        int W_npq;
        int C_npq;
        int W_nq;
        int C_nq;

        int macMaxBe; // maximum backoff exponent for CSMA
        int macMinBe; // minimum backoff exponent for CSMA

        cModule *hostNode; // reference to this host node's module

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
         * @param virtualLinkID a integer which represents the application ID
         * @param macAddr a inet::MacAddress which determines to which queue the packet is pushed to
         * @return Packet is successfully added or not
         */
        bool add2Queue(inet::Packet *, MacAddress macAddr, int virtualLinkID);

        /**
         * A public member function to reset the private member variables to determine the current neighbor and virtualLinkId
         */
        void reset();
        /**
         * Determines the total queue size of a specific neighbor identified by its MAC address
         * @param macaddress a inet::MacAddress argument to search for that MacAddress in the queue
         * @return Summed-up size of the normal and priority queue with the input MacAddress or 0 if the queue with that MacAddress is not found
         */
        int getTotalQueueSizeAt(inet::MacAddress macAddress);
        int getTotalQueueSize();
        int getNumBurstyPktsInQueue();

        /**
         * A public member function to return the size of the currently used neighbor queue
         * @return Size of the currently used queue
         */
        int getCurrentNeighborQueueSize();

        int getCurrentVirtualLinkIDKey();
        /**
         * A public member function to return the first packet of the currently used neighbor queue
         * @return Pointer to the first packet of the currently used queue
         */
        inet::Packet* getCurrentNeighborQueueFirstPacket();
        /**
         * A public member function taking one inet::MacAddress object to set the currently used queue
         * @param addr a inet::MacAddress argument to the set the selected queue
         */
        void setSelectedQueue(MacAddress macAddr, int linkID);

        void flushQueue(MacAddress neighbor, int vlinkId);
        void flush6pQueue(MacAddress neighbor);

        /**
         * A public member function to get the total number of all packets in all queues
         * @return Total packets
         */
        int checkTotalQueueSize();
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
         * A public member function returning a pointer to the TschCSMA of the currently used neighbor
         * @return pointer to the TschCSMA
         */
        TschCSMA* getCurrentTschCSMA();

        void terminateTschCsmaWith(MacAddress neighborAddr);
        TschCSMA* getTschCsmaWith(MacAddress neighborAddr);


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
        /**
         * A public member function to decrement backoff window for all queues directed at destAddr
         */
        void updateAllBackoffWindows(inet::MacAddress destAddress, TschSlotframe* sf);
        /**
         * A public member funtion to return a pointer to the macToQueueMap container
         * @return pointer to the map container macToQueue
         */
        std::map<inet::MacAddress, std::map<int, std::list<inet::Packet *>*>*>* getMacToQueueMap();
        /**
         * A public member funtion to return a pointer to the backoffTable container
         * @return pointer to the map container macToQueue
         */
        std::map<inet::MacAddress,TschCSMA*>* getBackoffTable();
        /**
         * Creates a new list from type inet::Packet*
         * @return pointer to the new list
         */
        Queue createQueue();
        /**
         * Creates a new map with integer key and list with inet::Packet* as values
         * @return pointer to this new map
         */
        std::map<int, std::list<inet::Packet *>*>* createVirtualQueue();
        /**
         * Prints the values of this neighbor class
         */
        void printQueue();
        /**
         * Deletes all elements of this neighbor class
         */
        void clearQueue();

        int getMacMaxBe() { return this->macMaxBe; }

        void setVirtualQueue(MacAddress macAddr, int linkId);

        int getVirtualQueueSizeAt(MacAddress macAddress, int virtualLinkId);

        friend std::ostream& operator<<(std::ostream& out, Queue q) {
            for (auto pkt : *q)
                out << pkt->getFullName() << endl;
            return out;
        }

        std::string printPacketQueue(Queue q) {
            std::ostringstream out;

            for (auto pkt : *q)
                out << pkt->getFullName() << endl;

            return out.str();
        }

    protected:
        virtual void refreshDisplay() const override;
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void handleMessage(cMessage *) override;
};
}
#endif /* LINKLAYER_IEEE802154E_TSCHNEIGHBOR_H_ */
