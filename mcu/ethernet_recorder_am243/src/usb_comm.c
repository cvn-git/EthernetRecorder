#include "usb_comm.h"
#include "app_config.h"


// USB
#include <usb/cdn/include/usb_init.h>
#include "tusb.h"

// FreeRTOS
#include "FreeRTOS.h"
#include "task.h"

// Standard C
#include <string.h>
#include <ctype.h>


TaskHandle_t    taskTud = NULL;
StackType_t     taskTudStackBuffer[TASK_TUD_STACK_SIZE_WORDS];
StaticTask_t    taskTudBuffer;

TaskHandle_t    taskCdc = NULL;
StackType_t     taskCdcStackBuffer[TASK_CDC_STACK_SIZE_WORDS];
StaticTask_t    taskCdcBuffer;



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

void taskTudLoop(void *args)
{
    while (1)
    {
        tud_task();
    }
}

void taskCdcLoop(void *args)
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
}
