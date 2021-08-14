/*
 * TschCLSF.h
 *
 *  Created on: Aug 13, 2021
 *      Author: yevhenii
 */

#ifndef LINKLAYER_IEEE802154E_SIXTISCH_TSCHCLSF_H_
#define LINKLAYER_IEEE802154E_SIXTISCH_TSCHCLSF_H_

using namespace std;

#include "TschCLSF.h"
#include "TschMSF.h"
#include "RplDefs.h"
#include "Tsch6tischComponents.h"
#include "Tsch6topSublayer.h"
#include "../Ieee802154eMac.h"

class TschCLSF: public TschMSF {

public:
    virtual int numInitStages() const override { return 7; }
    void initialize(int stage) override;

    TschCLSF();
    ~TschCLSF();

private:
    const tsch6pSFID_t pSFID = SFID_CLSF;
    int crossLayerChOffset;
    SlotframeChunk crossLayerSlotRange;
    bool isDaisyChained;
    bool isDaisyChainFailed;
    int maxNum6pAttempts;
    int num6pRelocateAttempts;
    int slotframeChunkPad;

    bool isValidSlotframeChunk(SlotframeChunk ch) { return !(ch.start < 0 || ch.end < 0 || ch.start > ch.end || ch.end > pSlotframeLength); }
    bool isCrossLayerInfoAvailable() { return isValidSlotframeChunk(crossLayerSlotRange) && crossLayerChOffset != -1; }

    offset_t chooseCrossLayerChOffset();
    vector<offset_t> getAvailableSlotsInRange(int start, int end, int pad);
    vector<cellLocation_t> getNonDaisyChained(vector<cellLocation_t> cellList);

    // Functions overriden from the MSF
    void deleteCells(uint64_t nodeId, int numCells) override;
    int createCellList(uint64_t destId, vector<cellLocation_t> &cellList, int numCells) override;
    void handleSuccessRelocate(uint64_t sender, vector<cellLocation_t> cellList) override;
    void relocateCells(uint64_t neighbor, vector<cellLocation_t> relocCells) override;

    // Signals handling
    void receiveSignal(cComponent *src, simsignal_t id, long value, cObject *details) override;
    void handleDaisyChaining(SlotframeChunk advertisedChunk);
    void setBranchChannelOffset(int chOf);

protected:
    virtual void refreshDisplay() const override;
};


#endif /* LINKLAYER_IEEE802154E_SIXTISCH_TSCHCLSF_H_ */
