/*
 * ResaUdpUtils.h
 *
 *  Created on: Jun 21, 2022
 *      Author: yevhenii
 */

#ifndef APPLICATIONS_UDPAPP_RESAUDPUTILS_H_
#define APPLICATIONS_UDPAPP_RESAUDPUTILS_H_

#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

inline static simtime_t getPacketDelay(Packet *pk) {
    auto creationTimeTag = pk->peekData()->findTag<CreationTimeTag>();
    return simTime() - creationTimeTag->getCreationTime();
}

inline static L3Address getPacketSrcAddress(Packet *pk) {
    auto l3Addresses = pk->getTag<L3AddressInd>();
    return l3Addresses->getSrcAddress();
}



#endif /* APPLICATIONS_UDPAPP_RESAUDPUTILS_H_ */
