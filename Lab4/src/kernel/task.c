#include "kernel/task.h"

task_t* task_head = 0;
task_t* task_tail = 0;

// Function to insert a task into the task queue based on priority
int task_create_DF1(void (*callback)(void *), void* data, int priority) {
    if(priority < 0)
        return 0;

    // preserve data
    char *copy = simple_malloc(string_len((char*)data) + 1);
    if(copy == 0)
        return 0;
    string_copy(copy, (char*)data);

    task_t *cur = task_head;
    task_t *newTask = (task_t*)simple_malloc(sizeof(task_t));
    // malloc fail
    if(newTask == 0)
        return 0;

    // disable interrupt to protect critical section
    asm volatile(
        "msr daifset, 0xf;"
    );

    newTask->prev = 0;
    newTask->next = 0;
    newTask->callback = callback;
    newTask->data = (void*)copy;
    newTask->priority = priority;

    // this is the first task inserted into queue
    if(task_head == 0){
        task_head = newTask;
        task_tail = newTask;
        return 1;
    }

    while(1){ 
        // insert into appropiate location based on increase-order
        if(newTask->priority <= cur->priority){
            newTask->prev = cur->prev;
            if(cur->prev != 0)
                cur->prev->next = newTask;
            cur->prev = newTask;
            newTask->next = cur;
            // if it is inserted into the head of queue
            if(cur == task_head){
                task_head = newTask;
            }
            break;
        } 
        // traverse to last element
        else if(cur == task_tail){
            task_tail->next = newTask;
            newTask->prev = task_tail;
            task_tail = newTask;
            break;
        }
        cur = cur->next;
    }

    // enable all interrupt
    asm volatile(
        "msr daifclr, 0xf;"
    );
    //uart_puts((char*)task_head->data);

    return 1;
}

int task_create_DF0(void (*callback)(), int priority){
    if(priority < 0)
        return 0;

    task_t *cur = task_head;
    task_t *newTask = (task_t*)simple_malloc(sizeof(task_t));
    // malloc fail
    if(newTask == 0)
        return 0;

    // disable interrupt to protect critical section
    asm volatile(
        "msr daifset, 0xf;"
    );

    newTask->prev = 0;
    newTask->next = 0;
    newTask->callback = callback;
    newTask->data = (void*)0;
    newTask->priority = priority;

    // this is the first task inserted into queue
    if(task_head == 0){
        task_head = newTask;
        task_tail = newTask;
        // enable all interrupt
        asm volatile(
            "msr daifclr, 0xf;"
        );

        return 1;
    }

    while(1){ 
        // insert into appropiate location based on increase-order
        if(newTask->priority <= cur->priority){
            newTask->prev = cur->prev;
            if(cur->prev != 0)
                cur->prev->next = newTask;
            cur->prev = newTask;
            newTask->next = cur;
            // if it is inserted into the head of queue
            if(cur == task_head){
                task_head = newTask;
            }
            break;
        } 
        // traverse to last element
        else if(cur == task_tail){
            task_tail->next = newTask;
            newTask->prev = task_tail;
            task_tail = newTask;
            break;
        }
        cur = cur->next;
    }

    //uart_puts((char*)task_head->data);
    // enable all interrupt
    asm volatile(
        "msr daifclr, 0xf;"
    );

    return 1;
}

void task_callback(void){
    uart_puts("task_callback\n");
}

// Function to print the tasks in the task queue
void ExecTasks(void) {
    task_t* cur = task_head;

    while(cur != 0){
        task_head = cur->next;
        if(task_head != 0){
            task_head->prev = 0;
        }
        //uart_puts((char*)cur->data);
        cur->callback(cur->data);
        // free cur(not implemented)
        cur = cur->next;
    }

    // enable all interrupt
    asm volatile(
        "msr daifclr, 0xf;"
    );
}