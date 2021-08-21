/*
 * Cross-layer Scheduling Function.
 *
 * Copyright (C) 2021  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2021  Yevhenii Shudrenko
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
    virtual void deleteCells(uint64_t nodeId, int numCells) override;
    virtual int createCellList(uint64_t destId, vector<cellLocation_t> &cellList, int numCells) override;
    virtual void handleSuccessRelocate(uint64_t sender, vector<cellLocation_t> cellList) override;
    virtual void relocateCells(uint64_t neighbor, vector<cellLocation_t> relocCells) override;
    virtual void handlePacketEnqueued(uint64_t destId) override;

    // Signals handling
    void receiveSignal(cComponent *src, simsignal_t id, long value, cObject *details) override;
    void handleDaisyChaining(SlotframeChunk advertisedChunk);
    void setBranchChannelOffset(int chOf);

protected:
    virtual void refreshDisplay() const override;
};


#endif /* LINKLAYER_IEEE802154E_SIXTISCH_TSCHCLSF_H_ */
