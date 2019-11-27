#include "TschSFSB.h"

#include "tsch6topCtrlMsg_m.h"
//#include "TschMacWaic.h"
#include "../Ieee802154eMac.h"
//#include "ChannelState.h"
#include "Tsch6topSublayer.h"
#include "inet/networklayer/common/InterfaceTable.h"
#include <omnetpp.h>
#include "TschSpectrumSensing.h"

using namespace tsch;

Define_Module(TschSFSB);

TschSFSB::TschSFSB() {
}
TschSFSB::~TschSFSB() {
}

void TschSFSB::initialize(int stage) {
    if (pNodeId == 1337) {
        // QUICK FIX: TODO do this properly.
        EV_DETAIL << "Tsch6topSublayer: WirelessDataConcentrator doesn't need "
                     "a scheduling function" << std::endl;
        return;
    }
    if (stage == 0) {
        pNumChannels = 16;//getParentModule()->par("nbRadioChannels");
        pTschLinkInfo = (TschLinkInfo*) getParentModule()->getSubmodule("linkinfo");


        pTimeout = par("timeout");
        numSensingResults = par("numSensingResults");
        blacklistThreshold = par("blacklistThreshold");

        INIT_NUM_CELLS = par("initNumcells");
        MAX_RETRY_BACKOFF = par("maxRetryBackoff");
        disable = par("disable");
        if (MAX_RETRY_BACKOFF <= pTimeout) {
            EV_ERROR << "maxRetryBackoff MUST be larger than pTimeout!" << endl;
        }
        BLACKLIST_UPDATE_TIMEOUT = par("blacklistUpdateTimeout");
        int blLifetime = par("blacklistLifetime");
        BLACKLIST_LIFETIME = SimTime(blLifetime, SIMTIME_MS);

        if (strcmp(par("version"), "blind") == 0) {
            blindHopping = true;
        } else {
            if (strcmp(par("version"), "extended") == 0) {
                pTschBlacklistManager->setChannelSwapOffset(par("channelSwapOffset"));
            }
            blindHopping = false;
        }

        SENSE_INTERVAL = par("senseInterval");

        /* initialize rssiHistory to avoid checks in handleSpectrumSensingResult() */
        // TODO!! Hardcoded for now, read from config!
        int minChannel = 11;
        int maxChannel = 26;
        for (int i = minChannel; i <= maxChannel; ++i) {
            //TODO: The variable is excluded for now, needs to be added when channelstate is fixed
            //channelHistory[i] = {};
        }

        s_InitialScheduleComplete = registerSignal("initial_schedule_complete");
    } else if (stage == 5) {
        auto interfaceModule = dynamic_cast<InterfaceTable *>(getParentModule()->getParentModule()->getParentModule()->getParentModule()->getSubmodule("interfaceTable", 0));
        pNodeId = interfaceModule->getInterface(1)->getMacAddress().getInt();
        pSlotframeSize = 101; // TODO get from somewhere
        pTschBlacklistManager =
                dynamic_cast<TschBlacklistManager *>(getParentModule()->getParentModule()->getSubmodule(
                        "blacklistmanager", 0));
        pTsch6p = (Tsch6topSublayer*) getParentModule()->getSubmodule("sixtop");

    }
}

void TschSFSB::start() {
    Enter_Method_Silent();

    /**Get all nodes that are within communication range of @p nodeId.
    *        Note that this only works if all nodes have been initialized (i.e.
    *        maybe not in init() step 1!)**/
    auto mac = check_and_cast<Ieee802154eMac*>(getModuleByPath("^.^.mac"));
    auto neighbors = dynamic_cast<Tsch6topSublayer*>(getParentModule()->getSubmodule("sixtop", 0))->getNeighborsInRange(pNodeId,mac);

    auto sensing = check_and_cast<tsch::sixtisch::TschSpectrumSensing*>(getModuleByPath("^.sensing"));
    //sensing->startContinousSensing();
    // Disable the functionality of TschSFSB
    if(!disable){
        /* Send add requests for INIT_NUM_CELLS cells to all of our neighbors */
        for(auto & neighbor : neighbors) {
            // TODO: overprovision (suggest more cells than numCells)?
            initialScheduleComplete[neighbor] = false;
            tsch6topCtrlMsg* msg = new tsch6topCtrlMsg();
            msg->setKind(SEND_INITIAL_ADD);
            msg->setDestId(neighbor);
            EV_DETAIL << "sending inital add to " << neighbor << endl;
            /* schedule initial initial ADD to random backoff between
             * MAX_RETRY_BACKOFF and pTimeout */
            int backoff = intrand(MAX_RETRY_BACKOFF - pTimeout) + pTimeout;
            scheduleAt(simTime()+ SimTime(backoff, SIMTIME_MS), msg);
        }

        cMessage* doSense = new cMessage();
        doSense->setKind(DO_SENSE);
        scheduleAt(simTime()+par("startSenseTime"), doSense);

    }

}

void TschSFSB::handleMessage(cMessage* msg) {
    if (msg->isSelfMessage()) {
        if (msg->getKind() == SEND_INITIAL_ADD) {
            tsch6topCtrlMsg* ctrl = dynamic_cast<tsch6topCtrlMsg*>(msg);
            sendInitialAdd(ctrl->getDestId());
            delete msg;
        } else if (msg->getKind() == DO_SENSE) {


            scheduleAt(simTime()+SENSE_INTERVAL, msg);
        }
    }
}

tsch6pSFID_t TschSFSB::getSFID() {
    Enter_Method_Silent();
    return pSFID;
}

int TschSFSB::createCellList(uint64_t destId, std::vector<cellLocation_t> &cellList,
                             int numCells) {
    Enter_Method_Silent();

    if (cellList.size() != 0) {
        //opp_warning("celLList should be empty");
        EV_WARN << "celLList should be empty" << endl;
        return -EINVAL;
    }
    if (!reservedTimeOffsets[destId].empty()) {
        EV_ERROR << "reservedTimeOffsets should be empty when creating new cellList,"
                " is another transaction still in progress?" << endl;
    }

    int numCellsChecked = 0;
    int cellsChecked[pSlotframeSize];

    for (int i = 0; i < pSlotframeSize; i++) {
        cellsChecked[0] = 0;
    }

    EV_DETAIL << "Node " << pNodeId << " picked ";
    while ((numCellsChecked < pSlotframeSize) &&
           ((int)cellList.size() < numCells)) {

        unsigned int timeOffset = intrand(pSlotframeSize);
        if (cellsChecked[timeOffset] == 0) {
            if ((timeOffsetReserved(timeOffset) == false) &&
                (pTschLinkInfo->timeOffsetScheduled(timeOffset) == false)
                // TODO: remove this check (bzw replace it with avoiding fixed cells),
                // just hard-coded this to see if rescheduling works in general
                && (timeOffset > 1)
                ) {
                /* timeOffset has not been proposed in any other transaction and
                   isn't part of a scheduled cell. Pick random ChannelOffset
                   and move on. */
                unsigned int channelOffset = intrand(pNumChannels);
                cellList.push_back({timeOffset, channelOffset});

                // TODO: give those entries a timeout?
                reservedTimeOffsets[destId].push_back(timeOffset);
                EV_DETAIL << "(" << timeOffset << ", " << channelOffset << "), " << endl;
            }

            numCellsChecked++;
            cellsChecked[timeOffset] = 1;
        }
    }

    EV_DETAIL << " for destination " << destId << std::endl;

    if (cellList.size() == 0) {
        return -ENOSPC;
    }

    if ((int)cellList.size() < numCells) {
        return -EFBIG;
    }

    return 0;
}

int TschSFSB::pickCells(uint64_t destId, std::vector<cellLocation_t> &cellList,
                        int numCells, bool isRX, bool isTX, bool isSHARED) {
    Enter_Method_Silent();

    if (cellList.size() == 0) {
        return -EINVAL;
    }

    std::vector<cellLocation_t>::iterator it = cellList.begin();
    int numCellsPicked = 0;
    for(; it != cellList.end() && numCellsPicked < numCells; ++it) {
        unsigned int timeOffset = it->timeOffset;
        if ((timeOffsetReserved(timeOffset) == false) &&
            (pTschLinkInfo->timeOffsetScheduled(timeOffset) == false)) {
            /* cell is still available. pick that. */
            numCellsPicked++;
        } else {
            /* we can't use that cell, remove it from cellList */
            cellList.erase(it, it);
        }
    }

    if (numCellsPicked == 0) {
        return -ENOSPC;
    }
    if (numCellsPicked < numCells) {
        return -EFBIG;
    }

    return 0;
}

void TschSFSB::handleResponse(uint64_t sender, tsch6pReturn_t code, int numCells,
                              std::vector<cellLocation_t> *cellList) {
    Enter_Method_Silent();

    // free the cells that were reserved during the transaction
    reservedTimeOffsets[sender].clear();

    if ((code == RC_SUCCESS) &&
        (initialScheduleComplete[sender] == false) &&
        (pTschLinkInfo->getLastKnownCommand(sender) == CMD_ADD) &&
        (pTschLinkInfo->getLastKnownType(sender) == MSG_RESPONSE)) {
        if (numCells == INIT_NUM_CELLS) {
            // initial ADD is complete
            // TODO: what if >0 but less than INIT_NUM_CELLS cells have been granted?
            // => request the rest!
            initialScheduleComplete[sender] = true;
            emit(s_InitialScheduleComplete, (unsigned long) sender);
            if (pendingBlacklistUpdates[sender].size() > 0) {
                /* activate the blacklist updates that we've buffered up until now */
                pTschBlacklistManager->updateNeighborBlacklist(sender,
                                                pendingBlacklistUpdates[sender]);
            }
        } else {
            EV_DETAIL << "TODO: send ADD request for remaining cells!" << std::endl;
        }

    }
    if (code == RC_RESET) {
        if (initialScheduleComplete[sender] == false) {
            // one of our initial ADD requests failed, try again

            tsch6topCtrlMsg* msg = new tsch6topCtrlMsg();
            msg->setKind(SEND_INITIAL_ADD);
            msg->setDestId(sender);
            /* get random backoff between MAX_RETRY_BACKOFF and pTimeout */
            int backoff = intrand(MAX_RETRY_BACKOFF - pTimeout) + pTimeout;
            scheduleAt(simTime()+ SimTime(backoff, SIMTIME_MS), msg);
        }
    }
}

void TschSFSB::handlePiggybackedData(uint64_t sender, void* data) {
    Enter_Method_Silent();

    if (blindHopping) {
        //opp_error("Blind Hopping Mode: We should not be getting piggybacked data!");
        EV_ERROR << "Blind Hopping Mode: We should not be getting piggybacked data!" << endl;
    }

    std::vector<TschBlacklistManager::blacklistEntry_t> blacklistUpdate;

    /* If this fails, it shouldn't fail silently because something went very wrong */
    blacklistUpdate_t* update = (blacklistUpdate_t*) data;
    bool hasStart  = update->header & TschBlacklistManager::HDRMASK_START;
    bool hasEnd    = update->header & TschBlacklistManager::HDRMASK_END;
    bool hasPeriod = update->header & TschBlacklistManager::HDRMASK_PERIOD;

    for (TschBlacklistManager::blacklistEntry_t entry: update->entries) {
        TschBlacklistManager::blacklistEntry_t newBlacklistEntry;
        newBlacklistEntry.channelNumber = entry.channelNumber;

        /* Set start/end/period fields if they are specified according to the header;
         * set them to default values otherwise. */
        newBlacklistEntry.start = hasStart? entry.start : simTime() ;
        newBlacklistEntry.end = hasEnd? entry.end: simTime() + BLACKLIST_LIFETIME;
        newBlacklistEntry.period = hasPeriod? entry.period: 0;

        blacklistUpdate.push_back(newBlacklistEntry);
    }

    if ((pTschLinkInfo->linkInfoExists(sender)) &&
        (pTschLinkInfo->getNumCells(sender) > 0)) {
        pTschBlacklistManager->updateNeighborBlacklist(sender, blacklistUpdate);
    } else {
        /* hold back the infos in this blacklist update until we have a confirmed link */
        pendingBlacklistUpdates[sender].insert(pendingBlacklistUpdates[sender].end(),
                                               blacklistUpdate.begin(), blacklistUpdate.end());
    }

    // TODO: figure out how to safely delete the update data!
    //delete update;
}

/*void TschSFSB::handleSpectrumSensingResult(tschSpectrumSensingResult* ssr) {
    Enter_Method_Silent();

    if (blindHopping) {
        delete ssr;
        return;
    }
*/
    /* collects the blacklist changes that will be sent to my neighbors */
//    blacklistUpdate_t* blacklistUpdate = new blacklistUpdate_t;
    /* By default, our blacklist entries don't have start/end/period fields */
//    blacklistUpdate->header = 0;

//    TschBlacklistManager::blacklistEntry_t newBlacklistEntry;
//    int updateSz = 0;
    //TODO: Needs to be updated, ChannelState is not used
    /**std::map<int, ChannelState>::iterator it;
    for (it = ssr->getSweepInfo().begin(); it != ssr->getSweepInfo().end(); ++it){
         store sensing result
        int channel = it->first;
        ChannelState state = it->second;

        if (state.isIdle() == false) {
            //std::cout << "xoxo " << channel << std::endl;
        }

        channelHistory[channel].push_back(state);
        if (channelHistory[channel].size() > numSensingResults) {
             vector was full before, remove oldest result
            channelHistory[channel].erase(channelHistory[channel].begin());
        }
         check if we should blacklist the channel
        double busyness = calcBusyness(channelHistory[channel]) ;
        if ((busyness >= blacklistThreshold) &&
            (pTschBlacklistManager->inPrivateBlacklist(channel, simTime())== false)) {

             Note: currently all BL entries start immediately, have the default
             * duration and no period. Therefore we don't send these values
             * explicitly (recipient will assume the default), but we still
             * need to add them to newBlacklistEntry for our private blacklist.
            newBlacklistEntry = {channel, 0, 0, 0};
            newBlacklistEntry.start = simTime();
            newBlacklistEntry.end = simTime() + BLACKLIST_LIFETIME;

            updateSz += blacklistupdateHdrSz + blacklistupdateChannelSz;

            blacklistUpdate->entries.push_back(newBlacklistEntry);
        }
    }**/

//    if (blacklistUpdate->entries.size() > 0) {
//        /* update our private blacklist */
//        pTschBlacklistManager->updatePrivateBlacklist(blacklistUpdate->entries);
//
//        /* send blacklist update to all neighbors*/
//        std::vector<int> links = pTschLinkInfo->getLinks();
//
//        for (int nodeId: links) {
//            pTsch6p->piggybackData(nodeId, blacklistUpdate, updateSz,
//                                   BLACKLIST_UPDATE_TIMEOUT);
//        }
//    } else {
//        delete blacklistUpdate;
//    }
//
//    delete ssr;
//}

void TschSFSB::handleInconsistency(uint64_t destId, uint8_t seqNum) {
    Enter_Method_Silent();
    EV_WARN << "Houston, we have an inconsistency." << endl;
    /*free reserved cells already to avoid race conditions*/
    reservedTimeOffsets[destId].clear();

    pTsch6p->sendClearRequest(destId, pTimeout);
}

int TschSFSB::getTimeout() {
    Enter_Method_Silent();

    return pTimeout;
}

void TschSFSB::sendInitialAdd(uint64_t nodeId) {
    if (initialScheduleComplete[nodeId] == false) {
        if (pTschLinkInfo->inTransaction(nodeId) == false) {
            /* Only do this if the initial schedule hasn't been set up successfully
               in the meantime and we're also not in the process of setting it up */
            std::vector<cellLocation_t> cellList;

            // TODO: piggyback current blacklist!!
            createCellList(nodeId, cellList, INIT_NUM_CELLS);
            pTsch6p->sendAddRequest(nodeId, MAC_LINKOPTIONS_TX, INIT_NUM_CELLS,
                                    cellList, pTimeout);
        } else {
            /* currently in another transaction with the neighbor, try gain later */
            tsch6topCtrlMsg* msg = new tsch6topCtrlMsg();
            msg->setKind(SEND_INITIAL_ADD);
            msg->setDestId(nodeId);
            /* get random backoff between MAX_RETRY_BACKOFF and pTimeout */
            int backoff = intrand(MAX_RETRY_BACKOFF - pTimeout) + pTimeout;
            scheduleAt(simTime()+ SimTime(backoff, SIMTIME_MS), msg);
        }
    }
}

bool TschSFSB::timeOffsetReserved(offset_t timeOffset) {
    std::map<uint64_t, std::vector<offset_t>>::iterator nodes;

    for (nodes = reservedTimeOffsets.begin(); nodes != reservedTimeOffsets.end(); ++nodes) {
        return timeOffsetReservedByNode(nodes->first, timeOffset);
    }

    return false;
}

bool TschSFSB::timeOffsetReservedByNode(uint64_t nodeId, offset_t timeOffset) {
    std::vector<offset_t> offsetSet = reservedTimeOffsets[nodeId];
    return std::find(offsetSet.begin(), offsetSet.end(), timeOffset) != offsetSet.end();
}

/**double TschSFSB::calcBusyness(std::vector<ChannelState> states) {

    double numBusyStates = 0.0;

    for (ChannelState state: states) {
        if (state.isIdle() == false) {
            numBusyStates++;
        }
    }

    double result = (numBusyStates/states.size())*100;

    return result;
}**/
