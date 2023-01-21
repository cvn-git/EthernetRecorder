#ifndef ETH_REC_APP_CONFIG_H
#define ETH_REC_APP_CONFIG_H


#define TASK_TUD_PRIORITY                   (4)
#define TASK_TUD_STACK_SIZE_WORDS           (4096U)

#define TASK_CDC_PRIORITY                   (3)
#define TASK_CDC_STACK_SIZE_WORDS           (4096U)

#define TASK_RECORDING_PRIORITY             (2)
#define TASK_RECORDING_STACK_SIZE_WORDS     (4096U)

#define TASK_BACKGROUND_PRIORITY            (1)
#define TASK_BACKGROUND_STACK_SIZE_WORDS    (4096U)


#endif  // ETH_REC_APP_CONFIG_H
