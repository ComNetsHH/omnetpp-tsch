// * @brief       Self-messages for link timeouts
// *
// * @author      Lotte Steenbrink <lotte.steenbrink@tuhh.de>

cplusplus {{
#include <stdint.h>
}}

message tschLinkInfoTimeoutMsg {
    uint64_t nodeId;     // The node with whom the timed out transaction was shared
    uint8_t seqNum; // sequence number associated with the transaction that timed out
}