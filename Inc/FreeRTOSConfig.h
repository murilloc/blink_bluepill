/*
 * FreeRTOSConfig.h -- configuracao do kernel FreeRTOS para STM32F103 (Cortex-M3).
 *
 * Pontos importantes:
 *  - configCPU_CLOCK_HZ deve bater com o clock real configurado em main.c
 *    (rcc_clock_setup_in_hse_8mhz_out_72mhz() -> 72 MHz), senao o tick fica errado.
 *  - Os 3 handlers do kernel sao mapeados para os nomes CMSIS da vector table do Cube
 *    (SVC_Handler / PendSV_Handler / SysTick_Handler), que estao como weak no startup.
 *  - Heap: usar heap_4.c (definido no CMakeLists). Tamanho em configTOTAL_HEAP_SIZE;
 *    lembrar que a Blue Pill tem so 20 KB de RAM no total.
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_PREEMPTION            1
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0
#define configCPU_CLOCK_HZ              ( 72000000UL )   /* bate com o PLL no main */
#define configTICK_RATE_HZ             ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES            ( 5 )
#define configMINIMAL_STACK_SIZE        ( ( unsigned short ) 128 )
#define configTOTAL_HEAP_SIZE          ( ( size_t ) ( 10 * 1024 ) )
#define configMAX_TASK_NAME_LEN         ( 16 )
#define configUSE_16_BIT_TICKS          0
#define configIDLE_SHOULD_YIELD         1
#define configUSE_MUTEXES               1
#define configUSE_RECURSIVE_MUTEXES     1
#define configUSE_COUNTING_SEMAPHORES   1
#define configQUEUE_REGISTRY_SIZE       8

#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configSUPPORT_STATIC_ALLOCATION  0

#define configCHECK_FOR_STACK_OVERFLOW   0
#define configUSE_MALLOC_FAILED_HOOK     0

/* Software timers */
#define configUSE_TIMERS                 1
#define configTIMER_TASK_PRIORITY        ( 2 )
#define configTIMER_QUEUE_LENGTH         10
#define configTIMER_TASK_STACK_DEPTH     ( configMINIMAL_STACK_SIZE * 2 )

/* API opcional incluida */
#define INCLUDE_vTaskPrioritySet         1
#define INCLUDE_uxTaskPriorityGet        1
#define INCLUDE_vTaskDelete              1
#define INCLUDE_vTaskSuspend             1
#define INCLUDE_vTaskDelayUntil          1
#define INCLUDE_vTaskDelay               1
#define INCLUDE_xTaskGetSchedulerState   1

/* Interrupcoes Cortex-M3 / STM32 (4 bits de prioridade) */
#define configPRIO_BITS                              4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* Mapear os handlers do FreeRTOS para os nomes CMSIS da vector table do Cube */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

#endif /* FREERTOS_CONFIG_H */
