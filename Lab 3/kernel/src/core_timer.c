#include "core_timer.h"
#include "asm.h"
#include "mmio.h"
#include "uart.h"
#include "string.h"
#include "memory.h"
#include "exception.h"


static uint64_t _alarm_mode = 0;
static uint32_t _duration = 1;

static void
_set_timeout(uint32_t duration)
{
   uint64_t freq = asm_read_sysregister(cntfrq_el0);
   asm_write_sysregister(cntp_tval_el0, freq * duration);
}


void
core_timer_enable(uint32_t duration)
{
    /*
    core_timer_enable:
        mov x0, 1
        msr cntp_ctl_el0, x0 // enable
        
        mrs x0, cntfrq_el0
        msr cntp_tval_el0, x0 // set expired time

        mov x0, 2
        ldr x1, =CORE0_TIMER_IRQ_CTRL
        str w0, [x1] // unmask timer interrupt
    */

   _duration = (duration > 0) ? duration : 1;

   asm_write_sysregister(cntp_ctl_el0, 1);
    _set_timeout(duration);
   *CORE0_TIMER_IRQ_CTRL = (uint32_t) 2;       // unmask timer interrupt
}


void
core_timer_disable()
{
    asm_write_sysregister(cntp_ctl_el0, 0);     // disable
    *CORE0_TIMER_IRQ_CTRL = (uint32_t) 0;       // unmask timer interrupt
}


uint64_t
core_timer_get_current_time()
{
    uint64_t freq = asm_read_sysregister(cntfrq_el0);
    uint64_t cur_cnt = asm_read_sysregister(cntpct_el0);
    return cur_cnt / freq;
}


void 
core_timer_set_alarm(uint32_t duration)
{
    _alarm_mode = 0;
    core_timer_enable(duration);
}


// void core_timer_interrupt_handler()
// {
//     uart_line("core_timer_interrupt_handler");

//     if (_alarm_mode) {
//         core_timer_disable();
//     } else {
//         _set_timeout(_duration);
//     }
// }


typedef void (*timer_cb)(byteptr_t);

typedef struct time_event {
    struct time_event   *prev, *next;
    uint64_t    event_time;
    uint64_t    duration;
    // uint64_t    expire_time;
    timer_cb    callback;
    byte_t      message[32];
} time_event_t;

typedef time_event_t* eventptr_t;

static eventptr_t _q_head = 0, _q_tail = 0;

static eventptr_t 
_new_event(timer_cb cb, byteptr_t msg, uint64_t duration)
{
    eventptr_t new_event = (eventptr_t) malloc(sizeof(time_event_t));
    new_event->event_time = core_timer_get_current_time();
    new_event->duration = duration;
    new_event->callback = cb;
    new_event->prev = 0;
    new_event->next = 0;
    str_ncpy(new_event->message, msg, 32);
    return new_event;
}

static void
_q_add_event(eventptr_t event)
{
    uart_line("---- _q_add_event ----");
    exception_l1_disable();

    if (_q_head == 0) {
        _q_tail = _q_head = event;
        core_timer_enable(event->event_time + event->duration - core_timer_get_current_time());
    }
    else {

        eventptr_t cur = _q_head;

        while (cur && (cur->event_time + cur->duration) < (event->event_time + event->duration))
            cur = cur->next; 
        
        if (cur == _q_head) {
            event->next = _q_head;
            _q_head->prev = event;
            _q_head = event;
            _set_timeout(event->event_time + event->duration - core_timer_get_current_time());
        }
        
        else if (!cur) {
            event->prev = _q_tail;
            _q_tail->next = event;
            _q_tail = event;
        } 
        
        else {
            event->next = cur;
            event->prev = cur->prev;
            cur->prev->next = event;
            cur->prev = event;
        }

    }

    exception_l1_enable();
}

static void
_print_message(byteptr_t msg)
{
    uart_str("\ntimer message: ");
    uart_line(msg);
}


void 
core_timer_set_timeout(byteptr_t message, uint64_t duration)
{
    uart_line("---- core_timer_set_timeout ----");
    _q_add_event(_new_event(_print_message, message, duration));
}

void
core_timer_interrupt_handler()
{
    exception_l1_disable();

    _q_head->callback(_q_head->message);

    byte_t buffer[32];
    uint32_to_ascii(_q_head->event_time, buffer);
    uart_str("command time: ");
    uart_line(buffer);
    
    uint32_t cur_time = core_timer_get_current_time();
    uint32_to_ascii(cur_time, buffer);
    uart_str("current  time: ");
    uart_line(buffer);

    eventptr_t next = _q_head->next;

    if (next) {
        uart_line("---- core_timer_interrupt_handler ----");
        next->prev = 0;
        _q_head = next;
        core_timer_enable((next->event_time + next->duration) - core_timer_get_current_time());
    }
    else {
        _q_head = _q_tail = 0;
        core_timer_disable();
    }

    exception_l1_enable();
}