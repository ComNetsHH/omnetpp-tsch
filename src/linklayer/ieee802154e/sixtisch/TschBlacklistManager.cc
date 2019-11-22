#include "TschBlacklistManager.h"
//#include "TschMacWaic.h"
#include "../Ieee802154eMac.h"

using namespace tsch;

Define_Module(TschBlacklistManager);


TschBlacklistManager::TschBlacklistManager() {
}

TschBlacklistManager::~TschBlacklistManager() {
}

void TschBlacklistManager::initialize(int stage) {
    if (stage == 0) {
        s_CellReassignmentSuccessful = registerSignal("cell_reassignment_successful");
        s_BlacklistEntryUpdatedPrivate = registerSignal("blacklist_entry_updated_private");
        s_BlacklistEntryUpdatedNeighbor = registerSignal("blacklist_entry_updated_neighbor");

        auto module = getParentModule();
        //New
        //BaseMacLayer* mac = dynamic_cast<BaseMacLayer*>(module->getSubmodule("mac", 0));
        Ieee802154eMac* mac = dynamic_cast<Ieee802154eMac *>(module->getSubmodule(
                    "Ieee802154eMac", 0));
        //pNodeId = mac->getMACAddress().getInt();
        pNodeId = mac->interfaceEntry->getMacAddress().getInt();
        pNumChannels = module->getSubmodule("sublayer")->par("nbRadioChannels");

        CHANNEL_SWAP_OFFSET = 0;
    }
}

void TschBlacklistManager::setChannelSwapOffset(int channelSwapOffset) {
    CHANNEL_SWAP_OFFSET = channelSwapOffset;
}

void TschBlacklistManager::updatePrivateBlacklist(std::vector<blacklistEntry_t> entries) {
    Enter_Method_Silent();

    updateBlacklist(pNodeId, &privateBlacklist, &entries);

    /* record statistics */
    emit(s_BlacklistEntryUpdatedPrivate, pNodeId);
}

void TschBlacklistManager::updateNeighborBlacklist(int nodeId,
                                                   std::vector<blacklistEntry_t> entries) {
    Enter_Method_Silent();

    if (neighborBlacklists.find(nodeId) == neighborBlacklists.end()) {
        // no neighbor blacklist for nodeId yet: create new one
        std::vector<blacklistEntry_t> nbl;
        neighborBlacklists[nodeId] = nbl;
    }

    std::cout << "Node " << pNodeId << " updating neighbor blacklist with node "
              << nodeId << " at " << simTime().dbl() << std::endl;
    updateBlacklist(nodeId, &neighborBlacklists[nodeId], &entries);

    /* record statistics */
    emit(s_BlacklistEntryUpdatedNeighbor, nodeId);
}

bool TschBlacklistManager::channelIsBlacklisted(int nodeId, simtime_t time,
                                                int channelNumber) {
    Enter_Method_Silent();

    return (inPrivateBlacklist(channelNumber, time) ||
           inNeighborBlacklist(nodeId, channelNumber, time));
}

bool TschBlacklistManager::inPrivateBlacklist(int channelNumber, simtime_t time) {
    Enter_Method_Silent();

    return inBlacklist(&privateBlacklist, channelNumber, time);
}

bool TschBlacklistManager::inNeighborBlacklist(int nodeId, int channelNumber,
                                               simtime_t time) {
    Enter_Method_Silent();

    if (neighborBlacklists.find(nodeId) == neighborBlacklists.end()) {
        // no neighbor blacklist for nodeId, do nothing
        return false;
    }

    return inBlacklist(&neighborBlacklists[nodeId], channelNumber, time);
}

int TschBlacklistManager::findAlternateChannel(int nodeId, simtime_t time, int channel) {
    int result = -1;
    /* needed for statistics collection */
    bool success = false;

    if (CHANNEL_SWAP_OFFSET > 0) {
        /* note: this implementation relies on CHANNEL_SWAP_OFFSET being chosen in
           such a way that each alternate channel will be hit only once during
           the += loop */

        int numChannelsChecked = 0;
        int newChannel = channel;

        // TODO: channels are hardcoded for now, read them from ini!
        int minChannel = 11;
        int maxChannel = 26;

        while ((result == -1) && (numChannelsChecked < pNumChannels)) {
            newChannel += CHANNEL_SWAP_OFFSET;

            if (newChannel > maxChannel) {
                /* wraparound */
                newChannel = (newChannel % (maxChannel +1)) + minChannel;
            }
            if (!channelIsBlacklisted(nodeId, time, newChannel)) {
                /* found a new channel */
                result = newChannel;
                success = true;
            }

            numChannelsChecked++;
        }
    }

    emit(s_CellReassignmentSuccessful, success);
    return result;
}

void TschBlacklistManager::handleMessage(cMessage* msg) {
    if (msg->isSelfMessage()) {
        tschBlacklistExpiryMsg* expiryMsg = dynamic_cast<tschBlacklistExpiryMsg*> (msg);

        if (expiryMsg && (msg->getKind() == DELETE_ENTRY)) {
            /* a blacklist entry expired, delete it */
            int nodeId = expiryMsg->getNodeId();
            int channelNumber = expiryMsg->getChannelNumber();

            if (nodeId == pNodeId) {
                deleteFromBlacklist(&privateBlacklist, channelNumber);
            } else {
                std::cout << "Node " << pNodeId << " deleting neighbor blacklist entry with node "
                          << expiryMsg->getNodeId() << " at " << simTime().dbl() << std::endl;
                deleteFromBlacklist(&neighborBlacklists[nodeId], channelNumber);
            }

            delete msg;
        }
    }
}

bool TschBlacklistManager::inBlacklist(std::vector<blacklistEntry_t>* blacklist,
                                       int channelNumber, simtime_t time) {
    for (blacklistEntry_t entry: *blacklist) {
        if ((entry.channelNumber == channelNumber) &&
            (entry.start < time) &&
            (entry.end > time)) {
            return true;
        }
    }

    return false;
}

void TschBlacklistManager::updateBlacklist(int nodeId,
                                           std::vector<blacklistEntry_t>* blacklist,
                                           std::vector<blacklistEntry_t>* entries) {
    for (blacklistEntry_t newEntry: *entries) {
        bool entryExists = false;

        for (blacklistEntry_t existingEntry: *blacklist) {

            if ((newEntry.channelNumber == existingEntry.channelNumber) &&
                (newEntry.period == existingEntry.period)) {
                /* found an existing entry that might have to be extended */

                if (newEntry.start < existingEntry.start) {
                    existingEntry.start = newEntry.start;
                }
                if (newEntry.end > existingEntry.end) {
                    existingEntry.end = newEntry.end;
                    cancelEvent(existingEntry.expiryMsg);
                    scheduleAt(existingEntry.end, existingEntry.expiryMsg);
                }

                // TODO: also emit signal?
                entryExists = true;
                break;
            }
        }

        if (!entryExists) {
            /* make sure this entry will be deleted again when its time has passed */
            tschBlacklistExpiryMsg* msg = new tschBlacklistExpiryMsg();
            msg->setKind(DELETE_ENTRY);
            msg->setChannelNumber(newEntry.channelNumber);
            msg->setNodeId(nodeId);

            newEntry.expiryMsg = msg;
            blacklist->push_back(newEntry);

            scheduleAt(newEntry.end, msg);
        }
    }
}

void TschBlacklistManager::deleteFromBlacklist(std::vector<blacklistEntry_t>* blacklist,
                                               int channelNumber) {
    simtime_t now = simTime();

    std::vector<blacklistEntry_t>::iterator i;
    for (i = blacklist->begin(); i != blacklist->end(); ++i) {

        if (((*i).channelNumber == channelNumber) &&
            ((*i).end <= now)){
            blacklist->erase(i);
            return;
        }
    }
}

void TschBlacklistManager::printBlacklist(std::vector<blacklistEntry_t>* blacklist) {
    std::cout << "[ ";
    for (blacklistEntry_t entry: *blacklist) {
        std::cout << entry.channelNumber << ", ";
    }

    std::cout << " ]"<< std::endl;
}
