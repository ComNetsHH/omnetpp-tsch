/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright (C) 2019  Institute of Communication Networks (ComNets),
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

#include "TschVirtualLink.h"
#include "TschLink.h"

namespace tsch {

Register_Class(TschVirtualLink);

TschVirtualLink::~TschVirtualLink() {
}

std::string TschVirtualLink::str() const
{
    std::stringstream out;
    out << "[ " << this->getSlotOffset() << ", ";
        out << this->getChannelOffset() << " ] ";
    if (isNormal())
        out << "NORM ";
    if (isAdv())
        out << "ADV ";
    if (isAdvOnly())
        out << "ADVonly ";
    if (isTx())
        out << "TX ";
    if (isRtx())
        out << "ReTX ";
    if (isRx())
        out << "RX ";
    if (isShared())
        out << "SHARED ";
    if (isTimekeeping())
        out << "TIME ";
    out << getAddr().str();
    out << " Virtual Link ID:" << getVirtualLink();
    return out.str();
}

std::string TschVirtualLink::detailedInfo() const
{
    return std::string();
}

bool TschVirtualLink::equals(const TschVirtualLink& link) const
{
    return this->getSlotframe() == link.getSlotframe() && getSlotOffset() == link.getSlotOffset(); // TODO which fields to compare?
}

int TschVirtualLink::getVirtualLink() const {
    return this->virtualLinkId;
}

void TschVirtualLink::setVirtualLink(int virtualLinkId) {
    this->virtualLinkId = virtualLinkId;
}

bool TschVirtualLink::isRtx() const {
    return this->option_rtx;
}

void TschVirtualLink::setRtx(bool optionRtx) {
    this->option_rtx = optionRtx;
}
}

