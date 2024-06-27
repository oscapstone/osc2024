#include "../include/task.h"

task_t* task_head = 0;
task_t* task_tail = 0;

task_t * create_task(void (*callback)(void *), int priority){
    if(priority < 0){
        uart_puts("pririty should >= 0\n");
        return 0;
    }

    
    disable_interrupt();
    task_t *newTask = (task_t*)simple_malloc(sizeof(task_t));
    if(!newTask){
        uart_puts("Fail to create task: Memory allocation error\n");
        return 0;
    }

    //disable core0 timer interrupt, otherwise will stuck in here
    *((volatile unsigned int*)CORE0_TIMER_IRQ_CTRL) = (0x0);

    newTask->prev = 0;
    newTask->next = 0;
    newTask->callback = callback;
    newTask->data = (void *)0;
    newTask->priority = priority;
    enable_interrupt();

    return newTask;
}

int add_task_to_queue(task_t *newTask){
    
    disable_interrupt();
    //disable core0 timer interrupt, otherwise will stuck in here
    *((volatile unsigned int*)CORE0_TIMER_IRQ_CTRL) = (0x0);

    task_t *cur = task_head;

    if(task_head == 0){
        task_head = newTask;
        task_tail = newTask;
        return 0;
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
    enable_interrupt();

    return 1;

}


void test_tesk1(void){
    uart_puts("tesk1 finished\n");
}

void test_tesk2(void){
    uart_puts("tesk2 finished\n");
}

void test_tesk3(void){
    uart_puts("tesk3 finished\n");
}

// Function to print the tasks in the task queue
void ExecTasks(void) {
    task_t* cur = task_head;
    //uart_puts("exec\n");
    //if not empty
    while(cur != 0){
        task_head = cur->next;
        if(task_head != 0){
            task_head->prev = 0;
        }
        //uart_puts((char*)cur->data);
        cur->callback(cur->data);

        cur = cur->next;
    }
    //uart_puts("Finish all task\n");

    // enable all interrupt
    //enable_interrupt();
}