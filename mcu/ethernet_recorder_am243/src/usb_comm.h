#ifndef ETH_REC_USB_COMM_H
#define ETH_REC_USB_COMM_H

#include <FreeRTOS.h>

#include <stdint.h>


void initUsbComm();

uint32_t writeUsb(const void* data, uint32_t numBytes, TickType_t ticksToWait);


#endif  // ETH_REC_USB_COMM_H
