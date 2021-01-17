/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 * Constants defined by 6TiSCH specifications and data structures used in
 * implementations of 6TiSCH documents.
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

#ifndef __WAIC_TSCH6TISCHCOMPONENTS_H_
#define __WAIC_TSCH6TISCHCOMPONENTS_H_

#include "WaicCellComponents.h"
#include <bitset>

/** ID of gates between modules. typedef'd for readability. */
typedef int GateId;

/**
 * Types of Control Messages sent from the sixtisch module to the mac module
 */
typedef enum TschSixtischCtrlMsg {
    CTRLMSG_PATTERNUPDATE, /**< Changes for the hopping pattern (to be fed to
                                updateHoppingPattern()) */
} sixtischCtrlMsg_t;

/**
 * Identifiers of all possible types of 6p Messages
 */
typedef enum Tsch6pMessageTypes {
    MSG_REQUEST,
    MSG_RESPONSE,
    MSG_CONFIRMATION
} tsch6pMsg_t;

/**
 * 6p Command Identifiers
 */
typedef enum Tsch6pCommands {
    CMD_ADD = 1,
    CMD_DELETE,
    CMD_RELOCATE,
    CMD_COUNT,
    CMD_LIST,
    CMD_SIGNAL,
    CMD_CLEAR
} tsch6pCmd_t;

inline std::ostream& operator<<(std::ostream& out, const Tsch6pCommands cmd6p) {
    const char* s = 0;
#define PRINT_ENUM(p) case(p): s = #p; break;
    switch(cmd6p) {
        PRINT_ENUM(CMD_ADD);
        PRINT_ENUM(CMD_DELETE);
        PRINT_ENUM(CMD_RELOCATE);
        PRINT_ENUM(CMD_COUNT);
        PRINT_ENUM(CMD_LIST);
        PRINT_ENUM(CMD_SIGNAL);
        PRINT_ENUM(CMD_CLEAR);
    }
#undef PRINT_ENUM

    return out << s;
}


inline std::ostream& operator<<(std::ostream& os, std::map<cellLocation_t, double> &list)
{
    for (auto const &el: list)
        os << el.first << " - " << el.second << std::endl;

    return os;
}

inline std::ostream& operator<<(std::ostream& os, const std::vector<std::tuple<cellLocation_t, uint8_t>> &cellVector)
{

    for (auto const &el: cellVector) {
        auto opts = std::get<1>(el);
        std::bitset<8> x(opts);
        os << std::get<0>(el) << ": " << x << " (options) " << std::endl;
    }

    return os;
}


/**
 * 6p Return Code Identifiers
 */
typedef enum Tsch6pReturnCodes
{
    RC_SUCCESS,   /**< operation succeeded */
    RC_EOL,       /**< end of list */
    RC_ERROR,     /**< generic error */
    RC_RESET,     /**< critical error, reset */
    RC_VERSION,   /**< unsupported 6P version */
    RC_SFID,      /**< unsupported SFID */
    RC_SEQNUM,    /**< schedule inconsistency */
    RC_CELLLIST,  /**< cellList error */
    RC_BUSY,      /**< busy */
    RC_LOCKED,    /**< cells are locked */
} tsch6pReturn_t;

inline std::ostream& operator<<(std::ostream& out, const Tsch6pReturnCodes errc) {
    const char* s = 0;
#define PRINT_ENUM(p) case(p): s = #p; break;
    switch(errc) {
        PRINT_ENUM(RC_SUCCESS);
        PRINT_ENUM(RC_EOL);
        PRINT_ENUM(RC_ERROR);
        PRINT_ENUM(RC_RESET);
        PRINT_ENUM(RC_VERSION);
        PRINT_ENUM(RC_SFID);
        PRINT_ENUM(RC_SEQNUM);
        PRINT_ENUM(RC_CELLLIST);
        PRINT_ENUM(RC_BUSY);
        PRINT_ENUM(RC_LOCKED);
    }
#undef PRINT_ENUM

    return out << s;
}

/**
 * 6P Scheduling Function Identifiers
 */
typedef enum Tsch6pSFIDs
{
    SFID_SFX,  /**< see https://tools.ietf.org/html/draft-ietf-6tisch-6top-sfx */
    SFID_SFSB, // TODO: set to actual name of our SF
    SFID_MSF,
    SFID_TEST
} tsch6pSFID_t;

typedef std::vector<std::tuple<cellLocation_t, uint8_t>> cellVector;
typedef std::vector<cellLocation_t> cellListVector;

#endif /*__WAIC_TSCH6TISCHCOMPONENTS_H_*/
