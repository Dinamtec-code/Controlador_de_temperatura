/*
 * FreeRTOS V202212.00
 * RTOS for STM32F3 Cortex-M4F
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_PREEMPTION              1
#define configUSE_IDLE_HOOK               0
#define configUSE_TICK_HOOK               0
#define configCPU_CLOCK_HZ                ((uint32_t) 72000000)
#define configTICK_RATE_HZ                ((TickType_t) 1000)
#define configMAX_PRIORITIES              5
#define configMINIMAL_STACK_SIZE          ((uint16_t) 128)
#define configTOTAL_HEAP_SIZE             ((size_t) 4*1024)
#define configUSE_TRACE_FACILITY        0
#define configUSE_16_BIT_TICKS          0
#define configIDLE_SHOULD_YIELD         1

#define configKERNEL_UNINITIALIZED      0
#define configKERNEL_PROTECTED          1
#define configKERNEL_RUNNING            2

#define configUSE_MUTEXes               1
#define configUSE_COUNTING_SEMAPHORES   1
#define configUSE_RECURSIVE_MUTEXES      1
#define configUSE_QUEUE_SETS            0

extern uint32_t SystemCoreClock;
#define configCPU_CLOCK_HZ    (SystemCoreClock)

#define configMAX_TASK_NAME_LEN         (8)
#define configUSE_MALLOC_FAILED_HOOK    0
#define configUSE_TASK_NOTIFICATIONS    1

/* Co-routine related definitions */
#define configUSE_CO_ROUTINES           0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

#define configUSE_TIMERS                0
#define configTIMER_TASK_PRIORITY        (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH         10
#define configTIMER_TASK_STACK_DEPTH      (configMINIMAL_STACK_SIZE * 2)

/* CMSIS-RTOS v2 specific */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1

/* Cortex-M4F specific */
#define configPRIO_BITS                 4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   0x0F
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY   (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* Normal assert */
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/* Handlers provided by main application */
extern void vApplicationStackOverflowHook( TaskHandle_t xTask, portCHAR *pcTaskName );

#endif /* FREERTOS_CONFIG_H */