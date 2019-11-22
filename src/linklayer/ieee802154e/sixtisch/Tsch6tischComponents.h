/**
 * @brief Constants defined by 6TiSCH specifications and data structures used in
 *        implementations of 6TiSCH documents.
 *
 * @author Lotte Steenbrink <lotte.steenbrink@tuhh.de>
 */

#ifndef __WAIC_TSCH6TISCHCOMPONENTS_H_
#define __WAIC_TSCH6TISCHCOMPONENTS_H_

#include "WaicCellComponents.h"

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

/**
 * 6P Scheduling Function Identifiers
 */
typedef enum Tsch6pSFIDs
{
    SFID_SFX,  /**< see https://tools.ietf.org/html/draft-ietf-6tisch-6top-sfx */
    SFID_SFSB, // TODO: set to actual name of our SF
    SFID_TEST
} tsch6pSFID_t;

typedef std::vector<std::tuple<cellLocation_t, uint8_t>> cellVector;
typedef std::vector<cellLocation_t> cellListVector;

#endif /*__WAIC_TSCH6TISCHCOMPONENTS_H_*/
