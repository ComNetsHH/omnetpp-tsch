/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright (C) 2021  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2019  Leo Krueger
 *           (C) 2008  Andras Varga
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

    bool src_auto;
    bool src_xml;

    int channelOffset;
    int slotOffset;

    inet::MacAddress addr;

  private:
    // copying not supported: following are private and also left undefined
    TschLink(const TschLink& obj);
    TschLink& operator=(const TschLink& obj);

  protected:
    virtual void changed(int fieldCode);

  public:
    TschLink() : sf(nullptr), option_tx(true), option_rx(true), option_shared(true),
        option_timekeeping(false), type_normal(true), type_advertising(false),
        type_advertisingOnly(false), src_auto(false), src_xml(false), channelOffset(0), slotOffset(0),
        addr(inet::MacAddress::BROADCAST_ADDRESS) {}
    virtual ~TschLink();
    virtual std::string str() const override;
    virtual std::string detailedInfo() const OMNETPP5_CODE(override);
    virtual std::string slug() const;

//    friend std::ostream& operator<<(std::ostream& os, const TschLink& link) { os << link.str(); return os; }
//    friend std::ostream& operator<<(std::ostream& os, std::vector<TschLink *> links) {
//        for (auto l : links)
//            os << l->str() << std::endl;
//        return os;
//    }

    virtual bool operator==(const TschLink& link) const { return equals(link); }
    virtual bool operator!=(const TschLink& link) const { return !equals(link); }
    virtual bool equals(const TschLink& link) const;

    /** To be called by the slotframe when this link is added or removed from it */
    virtual void setSlotframe(TschSlotframe *sf) { this->sf = sf; }
    virtual TschSlotframe *getSlotframe() const { return sf; }

    virtual bool isValid() const { return true; }

    // field codes for changed()
    enum { F_OPTIONTX, F_OPTIONRX, F_OPTIONSHARED, F_OPTIONTIME, F_TYPENORMAL, F_TYPEADV, F_TYPEADVONLY, F_SRCAUTO, F_SRCXML, F_CHANOFF, F_SLOTOFF, F_ADDR };

    virtual const inet::MacAddress& getAddr() const {
        return addr;
    }

    virtual void setAddr(const inet::MacAddress& addr) {
        this->addr = addr;
        changed(F_ADDR);
    }

    virtual int getChannelOffset() const {
        return channelOffset;
    }

    virtual void setChannelOffset(int channelOffset) {
        this->channelOffset = channelOffset;
        changed(F_CHANOFF);
    }

    virtual bool isRx() const {
        return option_rx;
    }

    virtual void setRx(bool optionRx) {
        option_rx = optionRx;
        changed(F_OPTIONRX);
    }

    virtual bool isShared() const {
        return option_shared;
    }

    virtual void setShared(bool optionShared) {
        option_shared = optionShared;
        changed(F_OPTIONSHARED);
    }

    virtual bool isTimekeeping() const {
        return option_timekeeping;
    }

    virtual void setTimekeeping(bool optionTimekeeping) {
        option_timekeeping = optionTimekeeping;
        changed(F_OPTIONTIME);
    }

    virtual bool isTx() const {
        return option_tx;
    }

    virtual void setTx(bool optionTx) {
        option_tx = optionTx;
        changed(F_OPTIONTX);
    }

    virtual int getSlotOffset() const {
        return slotOffset;
    }

    virtual void setSlotOffset(int slotOffset) {
        this->slotOffset = slotOffset;
        changed(F_SLOTOFF);
    }

    virtual bool isAdv() const {
        return type_advertising;
    }

    virtual void setAdv(bool typeAdvertising) {
        type_advertising = typeAdvertising;
        changed(F_TYPEADV);
    }

    virtual bool isAdvOnly() const {
        return type_advertisingOnly;

    }

    virtual void setAdvOnly(bool typeAdvertisingOnly) {
        type_advertisingOnly = typeAdvertisingOnly;
        changed(F_TYPEADVONLY);
    }

    virtual bool isNormal() const {
        return type_normal;
    }

    virtual void setNormal(bool typeNormal) {
        type_normal = typeNormal;
        changed(F_TYPENORMAL);
    }

    virtual void setAuto(bool srcAuto) {
        src_auto = srcAuto;
        changed(F_SRCAUTO);
    }

    virtual bool isAuto() {
        return src_auto;
    }

    virtual void setXml(bool srcXml) {
        src_xml = srcXml;
        changed(F_SRCXML);
    }

    virtual bool isXml() {
        return src_xml;
    }
};

} // namespace tsch

#endif    // __TSCH_TSCHLINK_H

