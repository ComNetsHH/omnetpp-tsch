/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright (C) 2021  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2019  Leo Krueger, Louis Yin
 *           (C) 2004-2006 Andras Varga
 *           (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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

#ifndef __TSCH_TSCHVIRTUALLINK_H
#define __TSCH_TSCHVIRTUALLINK_H

#include "TschLink.h"

typedef enum VirtualLinkPriorities {
    LINK_PRIO_CONTROL = -2,
    LINK_PRIO_HIGH = -1,
    LINK_PRIO_NORMAL = 0,
    LINK_PRIO_LOW = 1
} linkPrio_t;

namespace tsch {

class TschVirtualLink: public TschLink {
private:
    int virtualLinkId;
    bool option_rtx;

  private:
    // copying not supported: following are private and also left undefined
    TschVirtualLink(const TschVirtualLink& obj);
    TschVirtualLink& operator=(const TschVirtualLink& obj);

  public:
    TschVirtualLink() : TschLink(), virtualLinkId(0),option_rtx(false) {}
    virtual ~TschVirtualLink();
    virtual std::string str() const override;
    virtual std::string detailedInfo() const OMNETPP5_CODE(override);

    bool operator==(const TschVirtualLink& link) const{ return equals(link); }
    bool operator!=(const TschVirtualLink& link) const{ return !equals(link); }
    bool equals(const TschVirtualLink& link) const;
    bool equals(const TschLink& link) const override { return TschLink::equals(link); }

    bool isValid() const override { return true; }

    // field codes for changed()
    enum { F_OPTIONTX, F_OPTIONRX, F_OPTIONSHARED, F_OPTIONTIME, F_OPTIONRTX, F_TYPENORMAL, F_TYPEADV, F_TYPEADVONLY, F_CHANOFF, F_SLOTOFF, F_ADDR, F_VIRTUALLINK };

    int getVirtualLink() const;

    void setVirtualLink(int virtualLinkId);

    bool isRtx() const;

    void setRtx(bool optionRtx);
};
}
#endif /* LINKLAYER_IEEE802154E_TSCHVIRTUALLINK_H_ */
