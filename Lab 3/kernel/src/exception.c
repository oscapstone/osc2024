#include "type.h"
#include "uart.h"
#include "asm.h"
#include "core_timer.h"
#include "irq.h"
#include "aux.h"
#include "memory.h"


void 
exception_l1_enable() {
    asm volatile("msr daifclr, 0xf");           // umask all DAIF, it's column D,A,I,F on the register PASATE 
}


void
exception_l1_disable()
{
    asm volatile("msr daifset, 0xf");           // mask all DAIF, it's column D,A,I,F on the register PASATE 
}


typedef void (*task_callback)(byteptr_t);

typedef struct task {
    struct task     *prev, *next;
    uint64_t        priority;
    uint64_t        duration;
    task_callback   callback;
    byteptr_t       arg;
} task_t;

typedef task_t* taskptr_t;

static taskptr_t _q_head = 0, _q_tail = 0;

static taskptr_t 
_new_task(task_callback cb, byteptr_t arg, uint32_t priority)
{
    taskptr_t new_task = (taskptr_t) malloc(sizeof(task_t));
    new_task->priority = priority;
    new_task->callback = cb;
    new_task->arg = arg;
    new_task->next = 0;
    new_task->prev = 0;
    return new_task;
}


static void 
_add_task(taskptr_t new_task)
{
    // exception_l1_disable();

    if (_q_head == 0) {
        _q_head = _q_tail = new_task;
    }
    else {
        taskptr_t cur = _q_head;

        while (cur && cur->priority > new_task->priority) {
            cur = cur->next;
        }

        if (cur == _q_head) {
            new_task->next = _q_head;
            _q_head->prev = new_task;
            _q_head = new_task;
        }

        else if (cur == 0) {
            new_task->prev = _q_tail;
            _q_tail->next = new_task;
            _q_tail = new_task;
        }

        else {
            new_task->next = cur;
            new_task->prev = cur->prev;
            cur->prev->next = new_task;
            cur->prev = new_task;
        }
    }
    
    // exception_l1_enable();
}

static void
_run_task()
{
    while (_q_head) {
        // uart_line("---- run_task ----");
        _q_head->callback(_q_head->arg);
        // exception_l1_disable();
        _q_head = _q_head->next;
        // if (_q_head) _q_head->prev = 0;
        // else _q_head = _q_tail = 0;
        // exception_l1_enable();
    }
    _q_head = _q_tail = 0;
}


static void 
_show_exception_infomation()
{
    uint64_t spsr = asm_read_sysregister(spsr_el1);
    uint64_t elr  = asm_read_sysregister(elr_el1);
    uint64_t esr  = asm_read_sysregister(esr_el1);
    uart_str("spsr_el1: 0x"); uart_hexl(spsr); uart_endl();
    uart_str("elr_el1: 0x"); uart_hexl(elr); uart_endl();
    uart_str("esr_el1: 0x"); uart_hexl(esr); uart_endl();
}


void 
exception_invalid_handler()
{
    exception_l1_disable();
    uart_line("---- exception_invalid_handler ----");
    _show_exception_infomation();
    exception_l1_enable();
}


void      
exception_el1_sync_handler()
{
    exception_l1_disable();
    uart_line("---- exception_el1_sync_handler ----");
    _show_exception_infomation();
    exception_l1_enable();
}

// static void
// _el1_irq_handler()
// {
//     exception_l1_disable();
//     // uart_line("---- exception_el1_irq_handler ----");

//     uint32_t uart       = *IRQ_PENDING1 & (1 << 29);
//     uint32_t core_timer = *CORE0_INTERRUPT_SOURCE & 0x2;

//     if (uart) {
//         mini_uart_handler();
//     }
//     else if (core_timer) {
//         core_timer_interrupt_handler();
//     }

//     exception_l1_enable();  
// }

static uint64_t _busy = 0;

void
exception_el1_irq_handler()
{
    // uart_line("---- exception_el1_irq_handler ----");

    uint32_t uart       = *IRQ_PENDING1 & (1 << 29);
    uint32_t core_timer = *CORE0_INTERRUPT_SOURCE & 0x2;

    if (uart) {
        uint32ptr_t ier = (uint32ptr_t) malloc(sizeof(uint32_t));
        *ier = *AUX_MU_IER;
        _add_task(_new_task(mini_uart_interrupt_handler, (byteptr_t) ier, 0));
        aux_clr_rx_interrupt();
        aux_clr_tx_interrupt();
    }
    else if (core_timer) {
        uart_line("---- exception_el1_irq_handler ----");
        _add_task(_new_task(core_timer_interrupt_handler, 0, 2));
        core_timer_disable();
    }

    if (!_busy) {
        _busy = 1;
        _run_task();
        _busy = 0;
    }
}


void
exception_el0_sync_handler()
{
    exception_l1_disable();
    uart_line("---- exception_el0_sync_handler ----");
    _show_exception_infomation();
    exception_l1_enable();
}


void
exception_el0_irq_handler()
{
    exception_l1_disable();
    uart_line("---- exception_el0_irq_handler ----");
    _show_exception_infomation();
    exception_l1_enable();
}


