#ifndef LAYER2_RX_H
#define LAYER2_RX_H

#include "lwip/ip4_addr.h"


void custom_LwipifEnetApp_netifOpen(uint32_t netifIdx, const ip4_addr_t *ipaddr, const ip4_addr_t *netmask, const ip4_addr_t *gw);


#endif  // LAYER2_RX_H
