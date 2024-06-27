#include "sched.h"
#include "type.h"
#include "uart.h"
#include "list.h"
#include "frame.h"
#include "memory.h"
#include "delay.h"
#include "entry.h"
#include "shell.h"
#include "core_timer.h"
#include "exception.h"


// -------------------------------- //


/**
 *            p ->  +-----------------+ 0
 *                  |   task_struct   |
 *                  +-----------------+
 *                  |                 |             USER_STACK
 *                  |      STACK      |             
 *                  |                 |             USER_PROGRAM
 *    childregs ->  +-----------------+
 *                  |     pt_regs     |
 *                  +-----------------+ 4096
*/  


// #define THREAD_SIZE             0x1000
// #define STACK_SIZE              0x1000


void 		cpu_switch_to(void *, void *);


static task_t       init_task;

static task_ptr     current;
static task_ptr     shell_task;


int32_t 
get_pid()
{
    return current->pid;
}


task_ptr
current_task()
{
    return current;
}


task_ptr
get_shell_task()
{
    return shell_task;
}



/* *************************** *
 *       Scheduler OPs
 * *************************** */

static LIST_HEAD(running_queue);
static LIST_HEAD(waiting_queue);
static LIST_HEAD(stopped_queue);


static void
_preempt_disable()
{
    current->preempt_count++;
}


static void
_preempt_enable()
{
    current->preempt_count--;
}


static void 
_switch(task_ptr next)
{
	if (current == next) return;
	task_ptr prev = current;
	current = next;
    cpu_switch_to(&(prev->cpu_context), &(next->cpu_context));
}


static void
_schedule()
{
    _preempt_disable();

    int32_t c = 0;
    task_ptr next = 0;

    while (1) {
        c = -1;
        list_head_ptr_t p;

        // find next task
        list_for_each(p, &running_queue) {
            task_ptr tsk = (task_ptr) p;
            if (tsk->state == TASK_RUNNING && tsk->counter > c) {
                c = tsk->counter;
                next = tsk;
            }
        }

        if (c) { break; }  // found the next
        // else the list is empty, or no one is old enough

        // not found the next, do aging
        list_for_each(p, &running_queue) {
            task_ptr tsk = (task_ptr) p;
            tsk->counter = (tsk->counter >> 1) + tsk->priority;
        }
    }
    
    _switch(next);
    
    _preempt_enable();
}


void
schedule_tail()
{
    _preempt_enable();
}


/* 
 * called by the current task 
 *    e.g, idle() or process_exit(), ..etc. 
 * switch the context itself
 */
void
schedule()
{
    // reset the age of current task 
    current->counter = 0;
    _schedule();
}


/* 
 * called from irq handler
 * 
 */
void
scheduler_tick(byteptr_t _)
{
    // uart_printf("---- scheduler_tick ----\n");

    --current->counter;
	if (current->counter > 0 || current->preempt_count > 0) {
		return;
	}
	current->counter=0;
    enable_irq();
    _schedule();
    disable_irq();
}


static void
_pause_shell() 
{
    if (!shell_task || shell_task->state != TASK_RUNNING) return;
    shell_task->state = TASK_WAITING;
    list_del_entry((list_head_ptr_t) shell_task);
    list_add_tail((list_head_ptr_t) shell_task, &waiting_queue);
}


static void
_wake_shell()
{
    if (!shell_task || shell_task->state == TASK_RUNNING) return;
    shell_task->state = TASK_RUNNING;
    list_del_entry((list_head_ptr_t) shell_task);
    list_add_tail((list_head_ptr_t) shell_task, &running_queue);
}



void
scheduler_kill(task_ptr tsk, int32_t status)
{
    tsk->state = TASK_STOPPED;
    tsk->exitcode = status;
    if (tsk->user_stack)
        frame_release(tsk->user_stack);
    if (tsk->frame_count)
        frame_release_block(tsk->user_prog, tsk->frame_count);

    _preempt_disable();
    list_del_entry((list_head_ptr_t) tsk);
    list_add_tail((list_head_ptr_t) tsk, &stopped_queue);    
    if (tsk->foreground) _wake_shell();
    _preempt_enable();

    schedule();
}


task_ptr
scheduler_find(int32_t pid)
{
    list_head_ptr_t p;

    list_for_each(p, &running_queue) {
        task_ptr tsk = (task_ptr) p;
        if (tsk->pid == pid) return tsk;    
    }

    list_for_each(p, &waiting_queue) {
        task_ptr tsk = (task_ptr) p;
        if (tsk->pid == pid) return tsk;    
    }

    return 0;
}


void
scheduler_add(task_ptr tsk)
{
    tsk->state = TASK_RUNNING;
    _preempt_disable();
    list_add_tail((list_head_ptr_t) tsk, &running_queue);
    if (tsk->foreground) _pause_shell();
    _preempt_enable();
}


void
scheduler_init()
{   
    shell_task = 0;
    task_count = 0;
    task_init(&init_task);
    list_add_tail((list_head_ptr_t) &init_task, &running_queue);
 
    current = &(init_task);

    uart_printf("[DEBUG] sched_init - init_stask: 0x%x, pid=%d\n", &init_task, init_task.pid);
    core_timer_add_tick(scheduler_tick, "", 5);	
}


void
scheduler_add_shell(task_ptr tsk)
{
    tsk->state = TASK_RUNNING;
    _preempt_disable();
    list_add_tail((list_head_ptr_t) tsk, &running_queue);
    shell_task = tsk;
    _preempt_enable();
}


void
kill_zombies()
{
    _preempt_disable();
    while (!list_empty(&stopped_queue)) {
        task_ptr victim = (task_ptr) (&stopped_queue)->next;
        uart_printf("[DEBUG] killed victim: 0x%x, pid = %d\n", victim, victim->pid);
        list_del_entry((list_head_ptr_t) victim);
        frame_release((byteptr_t) victim);
    }
    _preempt_enable();
} 
