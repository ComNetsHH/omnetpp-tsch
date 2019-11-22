// * @brief       Self-messages for piggyback timeouts
// *
// * @author      Lotte Steenbrink <lotte.steenbrink@tuhh.de>

cplusplus {{
#include <stdint.h>
}}

message tschBlacklistExpiryMsg {
    uint64_t nodeId;     // The node with which the blacklist is shared (set to own address if this tschBlacklistExpiryMsg concerns private blacklist)
    int channelNumber; // channel whose entry just expired
}