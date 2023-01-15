#ifndef PACKET_RECORDER_H
#define PACKET_RECORDER_H

#include <lwip/netif.h>


void recordPacket(struct pbuf *p, struct netif *inp);


void initPacketRecorder();


#endif  // PACKET_RECORDER_H
