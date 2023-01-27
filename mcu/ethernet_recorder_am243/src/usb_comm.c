#include "usb_comm.h"
#include "app_config.h"


// Tiny USB
#include <class/cdc/cdc_device.h>
#include <device/usbd.h>

// FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// Standard C
#include <string.h>
#include <ctype.h>


#define MIN_FREE_TX_BYTES (32U)

TaskHandle_t    taskTud = NULL;
StackType_t     taskTudStackBuffer[TASK_TUD_STACK_SIZE_WORDS];
StaticTask_t    taskTudBuffer;

TaskHandle_t    taskCdc = NULL;
StackType_t     taskCdcStackBuffer[TASK_CDC_STACK_SIZE_WORDS];
StaticTask_t    taskCdcBuffer;

SemaphoreHandle_t   mutexCdc = NULL;
StaticSemaphore_t   mutexCdcBuffer;

SemaphoreHandle_t   semaphoreCdc = NULL;
StaticSemaphore_t   semaphoreCdcBuffer;


/* echo to either Serial0 or Serial1
   with Serial0 as all lower case, Serial1 as all upper case
 */
static void echo_serial_port(uint8_t itf, uint8_t buf[], uint32_t count)
{
    for(uint32_t i=0; i<count; i++)
    {
        if (itf == 0)
        {
            /* echo back 1st port as lower case */
            if (isupper(buf[i])) buf[i] += 'a' - 'A';
        }
        else
        {
            /* echo back additional ports as upper case */
            if (islower(buf[i])) buf[i] -= 'a' - 'A';
        }

        tud_cdc_n_write_char(itf, buf[i]);

        if ( buf[i] == '\r' ) tud_cdc_n_write_char(itf, '\n');
    }
    tud_cdc_n_write_flush(itf);
}

static void cdc_task(void)
{
    uint8_t itf;

#if 1
    static bool isConnected = false;
    if (tud_cdc_n_connected(0) != isConnected)
    {
        isConnected = !isConnected;
        if (isConnected)
        {
            DebugP_log("CDC #0 connected\r\n");
        }
        else
        {
            DebugP_log("CDC #0 disconnected\r\n");
        }
    }
#endif

    for (itf = 0; itf < CFG_TUD_CDC; itf++)
    {
        /* connected() check for DTR bit
           Most but not all terminal client set this when making connection
         */
        /* if ( tud_cdc_n_connected(itf) ) */
        {
            if ( tud_cdc_n_available(itf) )
            {
                uint8_t buf[64];

                uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));

                /* echo back to both serial ports */
                echo_serial_port(0, buf, count);
                echo_serial_port(1, buf, count);
            }
        }
    }
}

static void taskTudLoop(void *args)
{
    while (1)
    {
        tud_task();
    }
}

static void taskCdcLoop(void *args)
{
    while (1)
    {
        cdc_task();
        vTaskDelay(1);  // Short nap to avoid holding CPU
    }
}


void initUsbComm()
{
    /* TUD task is to handle the USB device events */
    taskTud = xTaskCreateStatic (
            taskTudLoop,
            "taskTud",                  // task name
            TASK_TUD_STACK_SIZE_WORDS,
            NULL,                       // pvParameters
            TASK_TUD_PRIORITY,
            taskTudStackBuffer,
            &taskTudBuffer);
    if (taskTud == NULL)
    {
        DebugP_logError("Cannot create TUD task\r\n");
        return;
    }

    /* CDC task is to handle the CDC class events */
    taskCdc = xTaskCreateStatic (
            taskCdcLoop,
            "taskCdc",                  // task name
            TASK_CDC_STACK_SIZE_WORDS,
            NULL,                       // pvParameters
            TASK_CDC_PRIORITY,
            taskCdcStackBuffer,
            &taskCdcBuffer);
    if (taskCdc == NULL)
    {
        DebugP_logError("Cannot create CDC task\r\n");
        return;
    }

    /* CDC mutex to queue writing commands */
    mutexCdc = xSemaphoreCreateMutexStatic(&mutexCdcBuffer);
    if (mutexCdc == NULL)
    {
        DebugP_logError("Cannot create CDC mutex\r\n");
        return;
    }

    /* Binary semaphore to wait for flushing CDC Tx */
    semaphoreCdc = xSemaphoreCreateBinaryStatic(&semaphoreCdcBuffer);
    if (semaphoreCdc == NULL)
    {
        DebugP_logError("Cannot create CDC semaphore\r\n");
        return;
    }
}

void tud_cdc_tx_complete_cb(uint8_t itf)
{
    // This module uses the first CDC interface only
    if (itf == 0)
    {
        xSemaphoreGive(semaphoreCdc);
    }
}

uint32_t writeUsb(const void* data, uint32_t numBytes, TickType_t ticksToWait)
{
    if (xSemaphoreTake(mutexCdc, ticksToWait) != pdTRUE)
    {
        return 0;
    }

    const uint8_t *bytesPtr = (const uint8_t *)data;
    uint8_t totalBytesWritten = 0;

    while (numBytes > 0)
    {
        if (!tud_cdc_n_connected(0))
        {
            // No USB connection, just discard the message
            break;
        }

        uint32_t bytesToWrite = tud_cdc_n_write_available(0);
        if (bytesToWrite < MIN_FREE_TX_BYTES)
        {
            // Wait until Tx FIFO has more space
            if (xSemaphoreTake(semaphoreCdc, ticksToWait) != pdTRUE)
            {
                break;
            }

            continue;
        }

        // Write to Tx FIFO
        if (bytesToWrite > numBytes)
        {
            bytesToWrite = numBytes;
        }
        if (tud_cdc_n_write(0, bytesPtr, bytesToWrite) != bytesToWrite)
        {
            DebugP_logError("tud_cdc_n_write failed\r\n");
            break;
        }
        tud_cdc_n_write_flush(0);

        // Update data buffer
        bytesPtr += bytesToWrite;
        totalBytesWritten += bytesToWrite;
        numBytes -= bytesToWrite;
    }

    if (xSemaphoreGive(mutexCdc) != pdTRUE)
    {
        DebugP_logError("Cannot release CDC mutex\r\n");
        return totalBytesWritten;
    }

    return totalBytesWritten;
}
