typedef void (*task_func_t)(void);
#define MAX_TASKS 64
typedef struct {
    task_func_t func;
    //void* data;
    int priority; // Optional for prioritized execution
} task_t;

typedef struct {
    task_t tasks[MAX_TASKS];
    int min_priority;
    int task_count;
} task_queue_t;
void create_task(task_func_t callback, unsigned int priority);
void execute_task();
void show_buffer();