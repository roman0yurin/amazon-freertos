/**
* @author Юрин Роман on 11.05.19.
* Набор определений, включающих почти полную трассировку жизненного цикла FreeRTOS
**/

#pragma once

#include "taskDecl.h"
#include "QueueDecl.h"


/**Called before a new task is selected to run. At this point pxCurrentTCB contains the handle of the task about to leave the Running state.**/
#define traceTASK_SWITCHED_OUT traceTASK_SWITCHED_OUT
void traceTASK_SWITCHED_OUT();

/*Called after a task has been selected to run. At this point pxCurrentTCB contains the handle of the task about to enter the Running state.*/
#define traceTASK_SWITCHED_IN traceTASK_SWITCHED_IN
void traceTASK_SWITCHED_IN();

/**Called when a task is transitioned into the Ready state.**/
#define traceMOVED_TASK_TO_READY_STATE(xTask) traceMOVED_TASK_TO_READY_STATE(xTask)
void traceMOVED_TASK_TO_READY_STATE(TaskHandle_t xTask);

/**Indicates that the currently executing task is about to block following an attempt to read from an empty queue, or an attempt to 'take' an empty semaphore or mutex.**/
#define traceBLOCKING_ON_QUEUE_RECEIVE(xQueue) traceBLOCKING_ON_QUEUE_RECEIVE(xQueue)
void traceBLOCKING_ON_QUEUE_RECEIVE(QueueHandle_t xQueue);

/**Indicates that the currently executing task is about to block following an attempt to write to a full queue.**/
#define traceBLOCKING_ON_QUEUE_SEND(xQueue) traceBLOCKING_ON_QUEUE_SEND_F(xQueue)
void traceBLOCKING_ON_QUEUE_SEND_F(QueueHandle_t xQueue);

/**Called from within xSemaphoreGiveRecursive()**/
#define traceGIVE_MUTEX_RECURSIVE(xMutex) traceGIVE_MUTEX_RECURSIVE(xMutex)
void traceGIVE_MUTEX_RECURSIVE(QueueHandle_t xMutex);


#define traceGIVE_MUTEX_RECURSIVE_FAILED(xMutex) traceGIVE_MUTEX_RECURSIVE_FAILED(xMutex)
void traceGIVE_MUTEX_RECURSIVE_FAILED(QueueHandle_t xMutex);

/**Called from within xQueueCreate() or xQueueCreateStatic() if the queue was successfully created.**/
#define traceQUEUE_CREATE(pxNewQueue) traceQUEUE_CREATE(pxNewQueue)
void traceQUEUE_CREATE(QueueHandle_t pxNewQueue);

#define traceQUEUE_CREATE_FAILED(ucQueueType) traceQUEUE_CREATE_FAILED(ucQueueType)
void traceQUEUE_CREATE_FAILED(uint8_t ucQueueType);

#define traceCREATE_MUTEX(pxNewMutex) traceCREATE_MUTEX(pxNewMutex)
void traceCREATE_MUTEX(QueueHandle_t pxNewMutex);


#define traceCREATE_MUTEX_FAILED traceCREATE_MUTEX_FAILED
void traceCREATE_MUTEX_FAILED();

#define traceTAKE_MUTEX_RECURSIVE(xMutex) traceTAKE_MUTEX_RECURSIVE(xMutex)
void traceTAKE_MUTEX_RECURSIVE(QueueHandle_t xMutex);

#define traceCREATE_COUNTING_SEMAPHORE() traceCREATE_COUNTING_SEMAPHORE()
void traceCREATE_COUNTING_SEMAPHORE();

#define traceCREATE_COUNTING_SEMAPHORE_FAILED() traceCREATE_COUNTING_SEMAPHORE_FAILED()
void traceCREATE_COUNTING_SEMAPHORE_FAILED();


#define traceQUEUE_SEND(xQueue) traceQUEUE_SEND(xQueue)
void traceQUEUE_SEND(QueueHandle_t xQueue);

#define traceQUEUE_SEND_FAILED(xQueue) traceQUEUE_SEND_FAILED(xQueue)
void traceQUEUE_SEND_FAILED(QueueHandle_t xQueue);

#define traceQUEUE_RECEIVE(xQueue) traceQUEUE_RECEIVE(xQueue)
void traceQUEUE_RECEIVE(QueueHandle_t xQueue);

#define traceQUEUE_RECEIVE_FAILED(xQueue) traceQUEUE_RECEIVE_FAILED(xQueue)
void traceQUEUE_RECEIVE_FAILED(QueueHandle_t xQueue);

#define traceQUEUE_PEEK(xQueue) traceQUEUE_PEEK(xQueue)
void traceQUEUE_PEEK(QueueHandle_t xQueue);

#define traceQUEUE_SEND_FROM_ISR(xQueue) traceQUEUE_SEND_FROM_ISR(xQueue)
void traceQUEUE_SEND_FROM_ISR(QueueHandle_t xQueue);

#define traceQUEUE_SEND_FROM_ISR_FAILED(xQueue) traceQUEUE_SEND_FROM_ISR_FAILED(xQueue)
void traceQUEUE_SEND_FROM_ISR_FAILED(QueueHandle_t xQueue);

#define traceQUEUE_RECEIVE_FROM_ISR(xQueue) traceQUEUE_RECEIVE_FROM_ISR(xQueue)
void traceQUEUE_RECEIVE_FROM_ISR(QueueHandle_t xQueue);

#define traceQUEUE_RECEIVE_FROM_ISR_FAILED(xQueue) traceQUEUE_RECEIVE_FROM_ISR_FAILED(xQueue)
void traceQUEUE_RECEIVE_FROM_ISR_FAILED(QueueHandle_t xQueue);

#define traceQUEUE_DELETE(xQueue) traceQUEUE_DELETE(xQueue)
void traceQUEUE_DELETE(QueueHandle_t xQueue);

#define traceTASK_CREATE(xTask) traceTASK_CREATE(xTask)
void traceTASK_CREATE(TaskHandle_t xTask);

#define traceTASK_CREATE_FAILED(pxNewTCB) traceTASK_CREATE_FAILED(pxNewTCB)
void traceTASK_CREATE_FAILED(void *pxNewTCB);

#define traceTASK_DELETE(xTask) traceTASK_DELETE(xTask)
void traceTASK_DELETE(TaskHandle_t xTask);

#define traceTASK_DELAY_UNTIL(tick) traceTASK_DELAY_UNTIL(tick)
void traceTASK_DELAY_UNTIL(size_t tick);

#define traceTASK_DELAY() traceTASK_DELAY()
void traceTASK_DELAY();

#define traceTASK_PRIORITY_SET(xTask,uxNewPriority) traceTASK_PRIORITY_SET(xTask,uxNewPriority)
void traceTASK_PRIORITY_SET(TaskHandle_t xTask, uint16_t uxNewPriority);

#define traceTASK_SUSPEND(xTask) traceTASK_SUSPEND(xTask)
void traceTASK_SUSPEND(TaskHandle_t xTask);

#define traceTASK_RESUME(xTask) traceTASK_RESUME(xTask)
void traceTASK_RESUME(TaskHandle_t xTask);

#define traceTASK_RESUME_FROM_ISR(xTask) traceTASK_RESUME_FROM_ISR(xTask)
void traceTASK_RESUME_FROM_ISR(TaskHandle_t xTask);

#define traceTIMER_COMMAND_RECEIVED(pxTimer, xCommandID, xCommandValue) traceTIMER_COMMAND_RECEIVED(pxTimer, xCommandID, xCommandValue)
void traceTIMER_COMMAND_RECEIVED(void *pxTimer, size_t xCommandID, size_t xCommandValue);

#define traceTIMER_COMMAND_SEND(pxTimer, xCommandID, xOptionalValue, xStatus) traceTIMER_COMMAND_SEND(pxTimer, xCommandID, xOptionalValue, xStatus)
void traceTIMER_COMMAND_SEND(void *pxTimer, size_t xCommandID, size_t xCommandValue, size_t xStatus);

#define traceTIMER_CREATE(pxNewTimer) traceTIMER_CREATE(pxNewTimer)
void traceTIMER_CREATE(void * pxNewTimer);

#define traceTIMER_CREATE_FAILED() traceTIMER_CREATE_FAILED()
void traceTIMER_CREATE_FAILED();

#define traceTIMER_EXPIRED(pxTimer) traceTIMER_EXPIRED(pxTimer)
void traceTIMER_EXPIRED(void *pxTimer);