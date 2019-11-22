// * @brief       Control messages sent by the 6top sublayer to the mac layer
// *
// * @author      Lotte Steenbrink <lotte.steenbrink@tuhh.de>


cplusplus {{
#include "WaicCellComponents.h"
#include "Tsch6tischComponents.h"

#include <stdint.h>
#include <vector>

typedef macLinkOption_t macLinkOption;
}}

class cellListVector { @existingClass;}
enum macLinkOption_t;
//class macLinkOption { @existingClass; }


message tsch6topCtrlMsg {
    simtime_t timeout;           // The absolute time at which this message
                                 // becomes invalid
    uint64_t destId;                  // The neighbor affected by the pattern update
    cellListVector newCells;     // The cells to be added
    cellListVector deleteCells;  // The cells to be added
    int cellOptions @enum(macLinkOption_t); // The linkOptions that apply to all links
                                 // in cellList
}