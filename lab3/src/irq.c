#include "irq.h"
#include "alloc.h"
#include "timer.h"
#include "uart.h"

typedef struct __irq_task_t {
    void (*func)();
    int priority;
    int dirty; // Set to 1 if the task is under processing
    struct __irq_task_t *prev;
    struct __irq_task_t *next;
} irq_task_t;

static irq_task_t *head = 0;

// Note: call this function with interrupt disabled!
void irq_add_task(void (*callback)(), int priority)
{
    irq_task_t *task = (irq_task_t *)simple_malloc(sizeof(irq_task_t));
    task->func = callback;
    task->priority = priority;
    task->dirty = 0;
    task->prev = 0;
    task->next = 0;

    if (head == 0 || task->priority < head->priority) {
        task->next = head;
        task->prev = 0;
        if (head != 0)
            head->prev = task;
        head = task;
        return;
    }

    irq_task_t *current = head;
    while (current->next != 0 && current->next->priority <= task->priority)
        current = current->next;
    task->next = current->next;
    if (current->next != 0)
        current->next->prev = task;
    current->next = task;
    task->prev = current;
}

void enable_interrupt()
{
    // Clear the D, A, I, F bits in the DAIF register
    // to enable interrupts in EL1
    asm volatile("msr DAIFClr, 0xF;");
}

void disable_interrupt()
{
    // Set 1 to the D, A, I, F bits in the DAIF register
    // to disable interrupts in EL1
    asm volatile("msr DAIFSet, 0xF;");
}

void irq_entry()
{
    // Enter the critical section ->
    // temporarily disable interrupts
    disable_interrupt();

    if (*IRQ_PENDING_1 & (1 << 29)) { // UART interrupt
        switch (*AUX_MU_IIR & 0x6) {
        case 0x2: // Transmit interrupt
            uart_disable_tx_interrupt();
            irq_add_task(uart_tx_irq_handler, 0);
            break;
        case 0x4: // Receive interrupt
            uart_disable_rx_interrupt();
            irq_add_task(uart_rx_irq_handler, 0);
            break;
        }
    } else if (*CORE0_INTERRUPT_SOURCE & 0x2) { // Core timer interrupt
        timer_disable_interrupt();
        irq_add_task(timer_irq_handler, 1);
    }

    enable_interrupt(); // Leave the critical section

    // Preemption: run the task with the highest priority
    while (head != 0 && head->dirty == 0) {
        // Get a task from queue
        disable_interrupt();
        irq_task_t *task = head;
        task->dirty = 1; // Flag the task as under processing
        enable_interrupt();

        task->func(); // Run the tasks with interrupts enabled

        // Remove the task from queue
        disable_interrupt();
        if (task->prev != 0)
            task->prev->next = task->next;
        if (task->next != 0)
            task->next->prev = task->prev;
        if (task == head)
            head = task->next;
        enable_interrupt();
    }
}