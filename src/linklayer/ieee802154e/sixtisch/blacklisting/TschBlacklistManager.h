/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 * Manages private and neighbor blacklists of a node based on the
 * information it is given by the scheduling function.
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

#ifndef __WAIC_TSCHBLACKLISTMANAGER_H_
#define __WAIC_TSCHBLACKLISTMANAGER_H_

#include <omnetpp.h>
#include "../WaicCellComponents.h"

#include "tschBlacklistExpiryMsg_m.h"

using namespace omnetpp;

class TschBlacklistManager: public cSimpleModule {
public:
    enum blacklistupdateHdrmask {
        HDRMASK_START = 0x04,
        HDRMASK_END = 0x02,
        HDRMASK_PERIOD = 0x01
    };

    /** @brief Entry in a private or neighbor blacklist. */
    struct blacklistEntry_t {
        int channelNumber;    /**< absolute(!!) channel number of the blacklisted channel */
        simtime_t start;      /**< The point in time at which the blacklisting applies */
        simtime_t end;        /**< The point in time at which this entry becomes invalid */
        simtime_t period;
        tschBlacklistExpiryMsg* expiryMsg;  /**< the self-message that will be triggered
                                                 when this entry expires */

        bool operator==(const blacklistEntry_t& other) const {
        return (channelNumber == other.channelNumber) &&
               (start == other.start) &&
               (end == other.end) &&
               (period == other.period);
        }
    };

    TschBlacklistManager();
    ~TschBlacklistManager();
    void initialize(int stage);

    /**
     * @brief set CHANNEL_SWAP_OFFSET
     */
    void setChannelSwapOffset(int channelSwapOffset);

    /**
     * @brief Add new/update existing entries in private blacklist.
     *        (since entries expire, there is no delete function)
     */
    void updatePrivateBlacklist(std::vector<blacklistEntry_t> entries);

    /**
     * @brief Copy new/update existing entries in neighbor blacklist for @p nodeId.
     *        (since entries expire, there is no delete function).
     */
    void updateNeighborBlacklist(int nodeId, std::vector<blacklistEntry_t> entries);

    /**
     * @brief Check whether the cell formed by <@p timeOffset, @p channelNumber>
     *        is in the shared blacklist of this node and @p nodeId.
     *
     * @param nodeId        The neighbor with whom the cell is shared
     * @param timeOffset    The absolute time at which the channel is (not) blacklisted
     * @param channelNumber The *absolute* channel number of the cell
     */
    bool channelIsBlacklisted(int nodeId, simtime_t time, int channelNumber);

    /**
     * ®brief Check if @p channelNumber is in our private blacklist at @p time,
     *        i.e. blacklistEntry.startingAt < @p time
     *        and  blacklistEntry.validUntil > @p time
     */
    bool inPrivateBlacklist(int channelNumber, simtime_t time);

    /**
     * ®brief Check if @p channelNumber is in our neighbor blacklist for
     *        @p nodeId at @p time,
     *        i.e. blacklistEntry.startingAt < @p time
     *        and  blacklistEntry.validUntil > @p time
     *
     * @return              true if @p channelNumber is blacklisted during @p time
     *                      false if it isn't blacklisted or there is no matching
     *                      blacklist entry
     */
    bool inNeighborBlacklist(int nodeId, int channelNumber, simtime_t time);

    /**
     * @brief find an alternative channel to use for link to @p nodeId at @p time
     *
     * @param channel       *absolute* channel number of the channel to be swapped
     *
     * @return              -1 if no suitable channel is found (mac SHOULD idle
     *                      in this case)
     *                      a channel number otherwise
     *
     */
    int findAlternateChannel(int nodeId, simtime_t time, int channel);

    void handleMessage(cMessage* msg);

private:
    enum blacklistSelfMessage_t {
        DELETE_ENTRY,
    };

    /** As defined by SFSB. Is 0 when SFSB is in simple mode or other SFs are used. */
    int CHANNEL_SWAP_OFFSET;

    /** Id of this node (i.e. the node operating this blacklistmanager) */
    int pNodeId;

    int pNumChannels;

    /** For statistics collection */
    simsignal_t s_CellReassignmentSuccessful;
    simsignal_t s_BlacklistEntryUpdatedPrivate;
    simsignal_t s_BlacklistEntryUpdatedNeighbor;

    /**
     * My own Blacklist (containing channels that this node has sensed
     * interference on)
     */
    std::vector<blacklistEntry_t> privateBlacklist;

    /**
     * (shared) Blacklists, indexed by neighbor ID.
     */
    std::map<int, std::vector<blacklistEntry_t>> neighborBlacklists;

    /**
     * @brief Check if @p channelNumber is in @p blacklist at @p time
     */
    bool inBlacklist(std::vector<blacklistEntry_t>* blacklist, int channelNumber,
                     simtime_t time);

    /**
     * @brief copy/update @p entries into @p blacklist.
     */
    void updateBlacklist(int nodeId, std::vector<blacklistEntry_t>* blacklist,
                         std::vector<blacklistEntry_t>* entries);

    /**
     * @brief Remove the entry for @p channelNumber from @p blacklist if
     *        entry.end < simTime()
     */
    void deleteFromBlacklist(std::vector<blacklistEntry_t>* blacklist,
                             int channelNumber);

    /* little helper */
    void printBlacklist(std::vector<blacklistEntry_t>* blacklist);
};


#endif /* __WAIC_TSCHBLACKLISTMANAGER_H_ */
