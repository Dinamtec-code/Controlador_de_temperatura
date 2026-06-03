#include "tasks/scheduler.h"
#include <string.h>

#define MAX_TASKS TASK_COUNT

static struct
{
    task_func_t func;
    uint32_t period_ms;
    uint32_t elapsed_ms;
    uint8_t enabled;
} tasks[MAX_TASKS];

static uint32_t system_tick_ms = 0;

void scheduler_init(void)
{
    memset(tasks, 0, sizeof(tasks));
}

void scheduler_add_task(task_id_t id, task_func_t func, uint32_t period_ms)
{
    if (id < MAX_TASKS)
    {
        tasks[id].func = func;
        tasks[id].period_ms = period_ms;
        tasks[id].elapsed_ms = 0;
        tasks[id].enabled = 1;
    }
}

void scheduler_run(void)
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (tasks[i].enabled && tasks[i].func)
        {
            if (tasks[i].elapsed_ms >= tasks[i].period_ms)
            {
                tasks[i].func();
                tasks[i].elapsed_ms = 0;
            }
        }
    }
}

void scheduler_systick_handler(void)
{
    system_tick_ms++;
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (tasks[i].enabled && tasks[i].period_ms > 0)
        {
            tasks[i].elapsed_ms += 1;
        }
    }
}

void scheduler_task_trigger(task_id_t id)
{
    if (id < MAX_TASKS && tasks[id].func)
    {
        tasks[id].func();
    }
}