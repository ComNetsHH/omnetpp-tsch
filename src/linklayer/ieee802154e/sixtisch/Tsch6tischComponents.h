/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH).
 * Constants defined by 6TiSCH specifications and data structures used in
 * implementations of 6TiSCH documents.
 *
 * Copyright (C) 2021  Institute of Communication Networks (ComNets),
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
#include "inet/linklayer/common/MacAddress.h"
#include <bitset>

/** To print enum 'key' as string */
#define PRINT_ENUM(p, s) case(p): s = #p; break;

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
    MSG_CONFIRMATION,
    MSG_NONE,
} tsch6pMsg_t;


inline std::ostream& operator<<(std::ostream& out, const macLinkOption_t macLinkOption) {
    const char* s = 0;
    switch(macLinkOption) {
        PRINT_ENUM(MAC_LINKOPTIONS_TX, s);
        PRINT_ENUM(MAC_LINKOPTIONS_RX, s);
        PRINT_ENUM(MAC_LINKOPTIONS_SHARED, s);
        PRINT_ENUM(MAC_LINKOPTIONS_PRIORITY, s);
        PRINT_ENUM(MAC_LINKOPTIONS_TIMEKEEPING, s);
        PRINT_ENUM(MAC_LINKOPTIONS_SRCAUTO, s);
        PRINT_ENUM(MAC_LINKOPTIONS_NONE, s);
    }
    return out << s;
}


inline std::ostream& operator<<(std::ostream& out, const Tsch6pMessageTypes tsch6p) {
    const char* s = 0;
    switch(tsch6p) {
        PRINT_ENUM(MSG_REQUEST, s);
        PRINT_ENUM(MSG_RESPONSE, s);
        PRINT_ENUM(MSG_CONFIRMATION, s);
        PRINT_ENUM(MSG_NONE, s);
    }
    return out << s;
}

inline std::string to_string(const Tsch6pMessageTypes tsch6p) {
    const char* s = 0;
    switch(tsch6p) {
        PRINT_ENUM(MSG_REQUEST, s);
        PRINT_ENUM(MSG_RESPONSE, s);
        PRINT_ENUM(MSG_CONFIRMATION, s);
        PRINT_ENUM(MSG_NONE, s);
    }
    std::string str(s);
    return str;
}

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
    CMD_CLEAR,
    CMD_NONE,
} tsch6pCmd_t;

inline std::ostream& operator<<(std::ostream& out, const Tsch6pCommands cmd6p) {
    const char* s = 0;
    switch(cmd6p) {
        PRINT_ENUM(CMD_ADD, s);
        PRINT_ENUM(CMD_DELETE, s);
        PRINT_ENUM(CMD_RELOCATE, s);
        PRINT_ENUM(CMD_COUNT, s);
        PRINT_ENUM(CMD_LIST, s);
        PRINT_ENUM(CMD_SIGNAL, s);
        PRINT_ENUM(CMD_CLEAR, s);
        PRINT_ENUM(CMD_NONE, s);
    }
    return out << s;
}

inline std::string to_string(const Tsch6pCommands cmd6p) {
    const char* s = 0;
    switch(cmd6p) {
        PRINT_ENUM(CMD_ADD, s);
        PRINT_ENUM(CMD_DELETE, s);
        PRINT_ENUM(CMD_RELOCATE, s);
        PRINT_ENUM(CMD_COUNT, s);
        PRINT_ENUM(CMD_LIST, s);
        PRINT_ENUM(CMD_SIGNAL, s);
        PRINT_ENUM(CMD_CLEAR, s);
        PRINT_ENUM(CMD_NONE, s);
    }
    std::string str(s);
    return str;
}


inline std::ostream& operator<<(std::ostream& os, std::map<cellLocation_t, double> &list)
{
    for (auto const &el: list)
        os << el.first << " - " << el.second << std::endl;

    return os;
}


inline std::string printLinkOptions(uint8_t op) {
    std::string outstr = "";
    if (getCellOptions_isRX(op))
        outstr += "RX ";
    if (getCellOptions_isTX(op))
        outstr += "TX ";
    if (getCellOptions_isSHARED(op))
        outstr += "SHARED ";
    if (getCellOptions_isAUTO(op))
        outstr += "AUTO ";
    return outstr;
}

inline std::ostream& operator<<(std::ostream& os, const std::vector<std::tuple<cellLocation_t, uint8_t>> &cellVector)
{
    for (auto const &link: cellVector)
        os << std::get<0>(link) << " " << printLinkOptions(std::get<1>(link)) << std::endl;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const std::list<uint64_t> macAddrList)
{
    for (auto const &addr: macAddrList)
        os << inet::MacAddress(addr) << ", ";
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
    switch (errc) {
        PRINT_ENUM(RC_SUCCESS, s);
        PRINT_ENUM(RC_EOL, s);
        PRINT_ENUM(RC_ERROR, s);
        PRINT_ENUM(RC_RESET, s);
        PRINT_ENUM(RC_VERSION, s);
        PRINT_ENUM(RC_SFID, s);
        PRINT_ENUM(RC_SEQNUM, s);
        PRINT_ENUM(RC_CELLLIST, s);
        PRINT_ENUM(RC_BUSY, s);
        PRINT_ENUM(RC_LOCKED, s);
    }
    return out << s;
}

/**
 * 6P Scheduling Function Identifiers
 */
typedef enum Tsch6pSFIDs
{
    SFID_SFX,  /**< see https://tools.ietf.org/html/draft-ietf-6tisch-6top-sfx */
    SFID_SFSB,
    SFID_MSF,
    SFID_CLSF,
    SFID_TEST
} tsch6pSFID_t;


inline std::ostream& operator<<(std::ostream& out, const Tsch6pSFIDs sfid) {
    const char* s = 0;
    switch(sfid) {
        PRINT_ENUM(SFID_SFX, s);
        PRINT_ENUM(SFID_SFSB, s);
        PRINT_ENUM(SFID_MSF, s);
        PRINT_ENUM(SFID_TEST, s);
        PRINT_ENUM(SFID_CLSF, s);
    }
    return out << s;
}

typedef std::vector<std::tuple<cellLocation_t, uint8_t>> cellVector;
typedef std::vector<cellLocation_t> cellListVector;

#endif /*__WAIC_TSCH6TISCHCOMPONENTS_H_*/
