// * @brief       Message type used by the minimal 6p implementation
// *
// * @author      Lotte Steenbrink <lotte.steenbrink@tuhh.de>

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

namespace tsch::sixtisch;

class SixpHeader extends inet::FieldsChunk {
    int version = 0;
    int type = -1;
    int code = -1;
    int sfid = -1;
    int seqNum = -1;
}