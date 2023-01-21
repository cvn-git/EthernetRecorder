#include "layer2_rx.h"
#include "packet_recorder.h"

#include <enet_apputils.h>
#include <ti_enet_lwipif.h>


static err_t  custom_tcpip_input(struct pbuf *p, struct netif *inp)
{
    recordPacket(p, inp);

    return tcpip_input(p, inp);
}


void custom_LwipifEnetApp_netifOpen(uint32_t netifIdx, const ip4_addr_t *ipaddr, const ip4_addr_t *netmask, const ip4_addr_t *gw)
{
    struct netif * const netIf = LwipifEnetApp_getNetifFromId(netifIdx);

    if(netifIdx < ENET_SYSCFG_NETIF_COUNT)
    {
#if ENET_CFG_IS_ON(CPSW_CSUM_OFFLOAD_SUPPORT)
        const uint32_t checksum_offload_flags = (NETIF_CHECKSUM_GEN_UDP | NETIF_CHECKSUM_GEN_TCP | NETIF_CHECKSUM_CHECK_TCP | NETIF_CHECKSUM_CHECK_UDP);
        const uint32_t checksum_flags = (NETIF_CHECKSUM_ENABLE_ALL & ~checksum_offload_flags);
#endif
        netif_add(  netIf,
                    ipaddr,
                    netmask,
                    gw,
                    NULL,
                    LWIPIF_LWIP_init,
                    custom_tcpip_input
                    );

        if(netifIdx == ENET_SYSCFG_DEFAULT_NETIF_IDX)
        {
            netif_set_default(netIf);
        }
#if ENET_CFG_IS_ON(CPSW_CSUM_OFFLOAD_SUPPORT)
       NETIF_SET_CHECKSUM_CTRL(netIf, checksum_flags);
#endif
    }
    else
    {
        DebugP_log("ERROR: NetifIdx is out of valid range!\r\n");
        EnetAppUtils_assert(FALSE);
    }
}
