/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 * Constants and data structures for WAIC MAC Cell code
 *
 * Copyright (C) 2019  Institute of Communication Networks (ComNets),
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

#ifndef __WAIC_WAICCELLCOMPONENTS_H_
#define __WAIC_WAICCELLCOMPONENTS_H_

#include <vector>
#include <map>
#include <cstdint>
#include <iterator>

typedef unsigned int offset_t;

/** The location of a cell, defined by <timeOffset, channelOffset> */
struct cellLocation_t {
    offset_t timeOffset;
    offset_t channelOffset;

    bool operator==(const cellLocation_t& other) const {
        return (timeOffset == other.timeOffset) && (channelOffset == other.channelOffset);
    }
    bool operator<(const cellLocation_t& other) const {
        return (timeOffset < other.timeOffset);
    }

    std::string toString()
    {
        return std::string("(") + std::to_string(timeOffset) + std::string(", ") + std::to_string(channelOffset) + std::string(")");
    }

    friend std::ostream& operator<<(std::ostream& os, cellLocation_t const& v)
    {
        os << "(" << v.timeOffset << ", " << v.channelOffset << ")";
        return os;
    }
};

template <typename T>
std::ostream& operator<< (std::ostream& out, const std::vector<T>& v) {
  if ( !v.empty() )
    std::copy (v.begin(), v.end(), std::ostream_iterator<T>(out, ", "));
  return out;
}

/* Bit masks for link options (see fig. 7-54 of the IEEE802.15.4e standard) */
enum macLinkOption_t {
    MAC_LINKOPTIONS_TX       = 0x01, /**< The link is a TX link */
    MAC_LINKOPTIONS_RX       = 0x02, /**< The link is a RX link */
    MAC_LINKOPTIONS_SHARED   = 0x04, /**< The link is a Shared link (can be
                                           combined with MAC_LINKOPTIONS_TX) */
    MAC_LINKOPTIONS_TIMEKEEPING = 0x08, /**< The link is to be used for clock
                                           synchronization. Shall be set to 1
                                           for MAC_LINKOPTIONS_RX. */
    MAC_LINKOPTIONS_PRIORITY = 0x10, /** The link is a priority channel access,
                                           as defined in section 6.2.5.2 of the
                                           IEEE802.15.4e standard*/
    MAC_LINKOPTIONS_SRCAUTO = 0x20,
    MAC_LINKOPTIONS_NONE = 0x23,

};

/**
 * @return             true if the TX bit in @p cellOptions is set,
 *                     false otherwise
 */
inline bool getCellOptions_isTX(uint8_t cellOptions) {
    return cellOptions & MAC_LINKOPTIONS_TX;
}

/**
 * @return             true if the RX bit in @p cellOptions is set,
 *                     false otherwise
 */
inline bool getCellOptions_isRX(uint8_t cellOptions) {
    return cellOptions & MAC_LINKOPTIONS_RX;
}

/**
 * @return             true if the SHARED bit in @p cellOptions is set,
 *                     false otherwise
 */
inline bool getCellOptions_isSHARED(uint8_t cellOptions) {
    return cellOptions & MAC_LINKOPTIONS_SHARED;
}

inline bool getCellOptions_isAUTO(uint8_t cellOptions) {
    return cellOptions & MAC_LINKOPTIONS_SRCAUTO;
}

typedef enum {
    INTERFERENCE_PROBABILITY /**< the probability that a link is interfered with. */
} metricType;

/**
 * The metric that denotes the predicted/measured quality of a cell.
 */
class WaicCellMetric {
public:
    metricType type;
    /*TODO: use uint between 0 and 100 instead? */
    float value;
};

#endif /* __WAIC_WAICCELLCOMPONENTS_H_ */
