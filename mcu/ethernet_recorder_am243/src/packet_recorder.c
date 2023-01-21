#include "packet_recorder.h"
#include "app_config.h"

// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

// Standard C
#include <string.h>


#define MAX_PACKET_SIZE 1600U
#define QUEUED_PACKETS 100U


typedef struct
{
    uint64_t        timestamp;
    struct netif*   netIf;
    uint32_t        numBytes;
    uint8_t         packetData[MAX_PACKET_SIZE];
} PacketRecord;


PacketRecord queuedPackets[QUEUED_PACKETS];

// Queue of free entries in queuedPackets
QueueHandle_t   queueFreeEntries = NULL;
uint32_t        queueFreeEntriesStorage[QUEUED_PACKETS];
StaticQueue_t   queueFreeEntriesBuffer;

// Queue of entries in queuedPackets ready for recording
QueueHandle_t   queueReadyEntries = NULL;
uint32_t        queueReadyEntriesStorage[QUEUED_PACKETS];
StaticQueue_t   queueReadyEntriesBuffer;

// Recording task
TaskHandle_t    taskRecording = NULL;
StackType_t     taskRecordingStackBuffer[TASK_RECORDING_STACK_SIZE_WORDS];
StaticTask_t    taskRecordingBuffer;


void recordPacket(struct pbuf *p, struct netif *inp)
{
    if (p->len > MAX_PACKET_SIZE)
    {
        return;
    }

    // This function is called from a task with very small stack. Be mindful of stack overflow here.

    uint32_t entryIdx = 0;
    if (xQueueReceive(queueFreeEntries, &entryIdx, portMAX_DELAY) != pdTRUE)
    {
        return;
    }
    PacketRecord* entry = &queuedPackets[entryIdx];

    entry->netIf = inp;
    entry->numBytes = p->len;
    memcpy(entry->packetData, p->payload, p->len);

    if (xQueueSendToBack(queueReadyEntries, &entryIdx, 0) != pdTRUE)
    {
        DebugP_logError("Invalid queue size\r\n");
        return;
    }
}


void packetRecordingTask(void *arg)
{
    uint32_t entryIdx = 0;
    while (1)
    {
        if (xQueueReceive(queueReadyEntries, &entryIdx, portMAX_DELAY) == pdTRUE)
        {
#if 0
            const PacketRecord* entry = &queuedPackets[entryIdx];
            DebugP_log("Receive %u bytes from the interface %X\r\n", entry->numBytes, entry->netIf);
#endif
            // Done. Return entryIdx to the free entry queue.
            if (xQueueSendToBack(queueFreeEntries, &entryIdx, 0) != pdTRUE)
            {
                DebugP_logError("Invalid queue size\r\n");
                return;
            }
        }
    }
}


void initPacketRecorder()
{
    // Create recording task
    taskRecording = xTaskCreateStatic (
            packetRecordingTask,
            "taskRecording",                    // task name
            TASK_RECORDING_STACK_SIZE_WORDS,
            NULL,                               // pvParameters
            TASK_RECORDING_PRIORITY,
            taskRecordingStackBuffer,
            &taskRecordingBuffer);
    if (taskRecording == NULL)
    {
        DebugP_logError("Cannot create packet recording task\r\n");
        return;
    }

    // Create free entry queue
    queueFreeEntries = xQueueCreateStatic(
            QUEUED_PACKETS,     // max items
            sizeof(uint32_t),   // item size
            (uint8_t *)queueFreeEntriesStorage,
            &queueFreeEntriesBuffer);
    if (queueFreeEntries == NULL)
    {
        DebugP_logError("Cannot create free entry queue\r\n");
        return;
    }

    // Create ready entry queue
    queueReadyEntries = xQueueCreateStatic(
            QUEUED_PACKETS,     // max items
            sizeof(uint32_t),   // item size
            (uint8_t *)queueReadyEntriesStorage,
            &queueReadyEntriesBuffer);
    if (queueReadyEntries == NULL)
    {
        DebugP_logError("Cannot create ready entry queue\r\n");
        return;
    }

    // Initialize free entry queue
    for (uint32_t k = 0; k < QUEUED_PACKETS; k++)
    {
        if (xQueueSendToBack(queueFreeEntries, &k, 0) != pdTRUE)
        {
            DebugP_logError("Invalid queue size\r\n");
            return;
        }
    }
}
