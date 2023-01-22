#ifndef ETH_REC_COMMON_H
#define ETH_REC_COMMON_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif


#define ETH_REC_SYNC_WORD (0x23490967U)
#define ETH_REC_HEADER_BYTES (16U)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t syncWord;          ///< Unique sync word
    uint16_t networkInterface;  ///< 0 or 1
    uint16_t numBytes;          ///< Number of bytes in the layer-2 Ethernet diagram
    uint64_t timestamp;         ///< Nanoseconds since the start of the MCU program
} EthRecHeader;

#ifdef __cplusplus
}   // extern "C"
#endif

#endif  // ETH_REC_COMMON_H
