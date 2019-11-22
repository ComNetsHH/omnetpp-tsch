// * @brief       Self-messages for piggyback timeouts
// *
// * @author      Lotte Steenbrink <lotte.steenbrink@tuhh.de>

cplusplus {{
#include <stdint.h>
}}

message tsch6pPiggybackTimeoutMsg {
    uint64_t destId;     // The node to which the data should be piggybacked
    int payloadSz;  // the size the payload would have in the "real" packet
                    // (in Byte)
    bool inTransit; // whether the data is currently being piggybacked
    // NOTE: payload is set as context Pointer!
}