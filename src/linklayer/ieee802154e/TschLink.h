//
// Copyright (C) 2008 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __TSCH_TSCHLINK_H
#define __TSCH_TSCHLINK_H

#include "inet/linklayer/common/MacAddress.h"
#include <omnetpp.h>

namespace tsch {

class TschSlotframe;

/**
 *
 */
class TschLink : public omnetpp::cObject
{
  private:
    TschSlotframe *sf;    ///< the slotframe in which the link is inserted or nullptr
    bool option_tx;
    bool option_rx;
    bool option_shared;
    bool option_timekeeping;    // TODO unused

    bool type_normal;
    bool type_advertising;      // TODO unused
    bool type_advertisingOnly;  // TODO unused

    int channelOffset;
    int slotOffset;

    inet::MacAddress addr;

  private:
    // copying not supported: following are private and also left undefined
    TschLink(const TschLink& obj);
    TschLink& operator=(const TschLink& obj);

  protected:
    void changed(int fieldCode);

  public:
    TschLink() : sf(nullptr), option_tx(true), option_rx(true), option_shared(true),
        option_timekeeping(false), type_normal(true), type_advertising(false),
        type_advertisingOnly(false), channelOffset(0), slotOffset(0),
        addr(inet::MacAddress::BROADCAST_ADDRESS) {}
    virtual ~TschLink();
    virtual std::string str() const override;
    virtual std::string detailedInfo() const OMNETPP5_CODE(override);

    bool operator==(const TschLink& link) const { return equals(link); }
    bool operator!=(const TschLink& link) const { return !equals(link); }
    bool equals(const TschLink& link) const;

    /** To be called by the slotframe when this link is added or removed from it */
    void setSlotframe(TschSlotframe *sf) { this->sf = sf; }
    TschSlotframe *getSlotframe() const { return sf; }

    bool isValid() const { return true; }

    // field codes for changed()
    enum { F_OPTIONTX, F_OPTIONRX, F_OPTIONSHARED, F_OPTIONTIME, F_TYPENORMAL, F_TYPEADV, F_TYPEADVONLY, F_CHANOFF, F_SLOTOFF, F_ADDR };

    const inet::MacAddress& getAddr() const {
        return addr;
    }

    void setAddr(const inet::MacAddress& addr) {
        this->addr = addr;
        changed(F_ADDR);
    }

    int getChannelOffset() const {
        return channelOffset;
    }

    void setChannelOffset(int channelOffset) {
        this->channelOffset = channelOffset;
        changed(F_CHANOFF);
    }

    bool isRx() const {
        return option_rx;
    }

    void setRx(bool optionRx) {
        option_rx = optionRx;
        changed(F_OPTIONRX);
    }

    bool isShared() const {
        return option_shared;
    }

    void setShared(bool optionShared) {
        option_shared = optionShared;
        changed(F_OPTIONSHARED);
    }

    bool isTimekeeping() const {
        return option_timekeeping;
    }

    void setTimekeeping(bool optionTimekeeping) {
        option_timekeeping = optionTimekeeping;
        changed(F_OPTIONTIME);
    }

    bool isTx() const {
        return option_tx;
    }

    void setTx(bool optionTx) {
        option_tx = optionTx;
        changed(F_OPTIONTX);
    }

    int getSlotOffset() const {
        return slotOffset;
    }

    void setSlotOffset(int slotOffset) {
        this->slotOffset = slotOffset;
        changed(F_SLOTOFF);
    }

    bool isAdv() const {
        return type_advertising;
    }

    void setAdv(bool typeAdvertising) {
        type_advertising = typeAdvertising;
        changed(F_TYPEADV);
    }

    bool isAdvOnly() const {
        return type_advertisingOnly;

    }

    void setAdvOnly(bool typeAdvertisingOnly) {
        type_advertisingOnly = typeAdvertisingOnly;
        changed(F_TYPEADVONLY);
    }

    bool isNormal() const {
        return type_normal;
    }

    void setNormal(bool typeNormal) {
        type_normal = typeNormal;
        changed(F_TYPENORMAL);
    }
};

} // namespace tsch

#endif    // __TSCH_TSCHLINK_H

