#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#define TASK_COMM_PERIOD_MS 5
#define TASK_SYSTEM_PERIOD_MS 5
#define TASK_CONTROL_PERIOD_MS 80
#define TASK_UI_PERIOD_MS 40

typedef enum
{
    TASK_COMM,
    TASK_SYSTEM,
    TASK_CONTROL,
    TASK_UI,
    TASK_COUNT
} task_id_t;

typedef void (*task_func_t)(void);

void scheduler_init(void);
void scheduler_add_task(task_id_t id, task_func_t func, uint32_t period_ms);
void scheduler_run(void);
void scheduler_systick_handler(void);
void scheduler_task_trigger(task_id_t id);

#endif