/**
* @author Юрин Роман on 11.05.19.
**/
#include "core/log/logger.h"
#if  TRACE_FREE_RTOS
extern "C"{
	#include "FreeRTOS.h"
	#include "RtosTracing.h"
	#include "task.h"
	#include <private/mpu_wrappers.h>
	#include <queue.h>


	void traceTASK_SWITCHED_OUT(){
		printfDebug("switch out %s", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()));
	}

	void traceTASK_SWITCHED_IN(){
		printfDebug("switch in %s", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()));
	}

	void traceMOVED_TASK_TO_READY_STATE(TaskHandle_t xTask){
		printfDebug("task %s ready", pcTaskGetTaskName(xTask));
	}

	void traceBLOCKING_ON_QUEUE_RECEIVE(QueueHandle_t xQueue){
		printfDebug("task %s wait receive from queue 0x%x", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xQueue));
	}

	void traceBLOCKING_ON_QUEUE_SEND_F(QueueHandle_t xQueue){
		printfDebug("task %s wait send to queue 0x%x", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xQueue));
	}

	void traceGIVE_MUTEX_RECURSIVE(QueueHandle_t xMutex){
		printfDebug("task %s give recursive mutex 0x%x", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xMutex));
	}

	void traceGIVE_MUTEX_RECURSIVE_FAILED(QueueHandle_t xMutex){
		printfInfo("task %s FAILED give recursive mutex 0x%x", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xMutex));
	}

	void traceQUEUE_CREATE(QueueHandle_t xQueue){
		printfInfo("task %s create queue 0x%x", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xQueue));
	}

	void traceQUEUE_CREATE_FAILED(uint8_t ucQueueType){
		printfSevere("task %s create FAILED queue, type 0x%x", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), ucQueueType);
	}

	void traceCREATE_MUTEX(QueueHandle_t pxNewMutex){
		printfInfo("task %s create mutex 0x%x", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(pxNewMutex));
	}

	void traceCREATE_MUTEX_FAILED(){
		printfSevere("task %s FAILED create FAILED mutex", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()));
	}

	void traceTAKE_MUTEX_RECURSIVE(QueueHandle_t xMutex){
		printfDebug("task %s take recursive mutex 0x%x", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xMutex));
	}

	void traceCREATE_COUNTING_SEMAPHORE(){
		printfInfo("task %s create count semaphore", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()));
	}

	void traceCREATE_COUNTING_SEMAPHORE_FAILED(){
		printfSevere("task %s create count semaphore FAILED", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()));
	}

	void traceQUEUE_SEND(QueueHandle_t xQueue){
		printfDebug("task %s send queue 0x%x", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xQueue));
	}

	void traceQUEUE_SEND_FAILED(QueueHandle_t xQueue){
		printfWarning("task %s send queue 0x%x FAILED", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xQueue));
	}

	void traceQUEUE_RECEIVE(QueueHandle_t xQueue){
		printfDebug("task %s receive queue 0x%x", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xQueue));
	}

	void traceQUEUE_RECEIVE_FROM_ISR(QueueHandle_t xQueue){
		printfDebug("ISR task %s receive queue 0x%x", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xQueue));
	}

	void traceQUEUE_RECEIVE_FROM_ISR_FAILED(QueueHandle_t xQueue){
		printfInfo("ISR task %s receive queue 0x%x FAILED", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xQueue));
	}

	void traceQUEUE_RECEIVE_FAILED(QueueHandle_t xQueue){
		printfInfo("task %s receive queue 0x%x FAILED", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xQueue));
	}

	void traceQUEUE_PEEK(QueueHandle_t xQueue){
		printfDebug("task %s receive peek 0x%x", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xQueue));
	}

	void traceQUEUE_SEND_FROM_ISR(QueueHandle_t xQueue){
		printfDebug("ISR send queue 0x%x", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xQueue));
	}

	void traceQUEUE_SEND_FROM_ISR_FAILED(QueueHandle_t xQueue){
		printfWarning("ISR send queue 0x%x FAILED", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xQueue));
	}

	void traceQUEUE_DELETE(QueueHandle_t xQueue){
		printfInfo("task %s delete queue 0x%x", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), reinterpret_cast<uintptr_t>(xQueue));
	}

  void traceTASK_CREATE(TaskHandle_t xTask){
		printfDebug("create task %s", pcTaskGetTaskName(xTask));
	}

	void traceTASK_CREATE_FAILED(void *pxNewTCB){
		printSevere("crate task FAILED");
	}

	void traceTASK_DELETE(TaskHandle_t xTask){
		printfInfo("delete task %s", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()));
	}

	void traceTASK_DELAY_UNTIL(size_t tick){
		printfDebug("task %s delay until %du", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), tick);
	}

	void traceTASK_DELAY(){
		printfDebug("task %s delay", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()));
	}

	void traceTASK_PRIORITY_SET(TaskHandle_t xTask, uint16_t uxNewPriority){
		printfDebug("task %s set priority %du", pcTaskGetTaskName(xTask), uxNewPriority);
	}

	void traceTASK_SUSPEND(TaskHandle_t xTask){
		printfDebug("suspend task %s", pcTaskGetTaskName(xTask));
	}

	void traceTASK_RESUME(TaskHandle_t xTask){
		printfDebug("resume task %s", pcTaskGetTaskName(xTask));
	}

	void traceTASK_RESUME_FROM_ISR(TaskHandle_t xTask){
		printfDebug("ISR resume task %s", pcTaskGetTaskName(xTask));
	}

	void traceTIMER_COMMAND_RECEIVED(void *pxTimer, size_t xCommandID, size_t xCommandValue){
		printfDebug("timer 0x%x received command %du value %du", reinterpret_cast<uintptr_t>(pxTimer), xCommandID, xCommandValue);
	}

	void traceTIMER_COMMAND_SEND(void *pxTimer, size_t xCommandID, size_t xCommandValue, size_t xStatus){
		printfDebug("timer 0x%x received command %du value %du status %du", reinterpret_cast<uintptr_t>(pxTimer), xCommandID, xCommandValue, xStatus);
	}

	void traceTIMER_CREATE(void * pxNewTimer){
		printfDebug("create timer 0x%x", reinterpret_cast<uintptr_t>(pxNewTimer));
	}

	void traceTIMER_CREATE_FAILED(){
		printSevere("create timer FAILED");
	}

	void traceTIMER_EXPIRED(void *pxTimer){
		printfWarning("timer 0x%x expired", reinterpret_cast<uintptr_t>(pxTimer));
	}
}

#endif //TRACE_FREE_RTOS


