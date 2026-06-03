#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#define TASK_PERIOD_MS 20

typedef enum {
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