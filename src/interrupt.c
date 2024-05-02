#include "interrupt.h"

#include "multitask.h"
#include "syscall_.h"
#include "task.h"
#include "timer.h"
#include "uart1.h"
#include "utli.h"

int32_t lock_cnt = 0;

void invaild_exception_handler() {
  uart_puts("invaild exception handler!");
  uart_puts("Rebooting...");
  reset(1000);
  while (1)
    ;
}

void el0_64_sync_interrupt_handler(trapframe_t *tpf) {
  enable_interrupt();
  uint64_t syscall_num = tpf->x[8];
  switch (syscall_num) {
    case SYSCALL_GET_PID:
      sys_getpid(tpf);
      break;
    case SYSCALL_UARTREAD:
      sys_uartread(tpf, (char *)tpf->x[0], (uint32_t)tpf->x[1]);
      break;
    case SYSCALL_UARTWRITE:
      sys_uartwrite(tpf, (const char *)tpf->x[0], (uint32_t)tpf->x[1]);
      break;
    case SYSCALL_EXEC:
      exec(tpf, (const char *)tpf->x[0], (char **const)tpf->x[1]);
      break;
    case SYSCALL_FORK:
      fork(tpf);
      break;
    case SYSCALL_EXIT:
      exit();
      break;
    case SYSCALL_MBOX:
      sys_mbox_call(tpf, (uint8_t)tpf->x[0], (uint32_t *)tpf->x[1]);
      break;
    case SYSCALL_KILL:
      kill((uint32_t)tpf->x[0]);
      break;
    case SYSCALL_SIG:
      signal((uint32_t)tpf->x[0], (sig_handler_func)tpf->x[1]);
      break;
    case SYSCALL_SIGKILL:
      sig_kill((uint32_t)tpf->x[0], (uint32_t)tpf->x[1]);
      break;
    case SIG_RETURN:
      sig_return();
      break;
    default:
      uart_puts("err: undefiend syscall number");
      break;
  }
}

static void timer_interrupt_handler() { timer_event_pop(); };

void irq_interrupt_handler() {
  if (*CORE0_INT_SRC & CORE_INT_SRC_TIMER) {
    set_core_timer_int(get_clk_freq() >> 5);
    add_task(timer_interrupt_handler, TIMER_INT_PRIORITY);
    pop_task();
    schedule();
  }
  if ((*CORE0_INT_SRC & CORE_INT_SRC_GPU) &&       //  (uart1_interrupt)
      (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT)) {  //  bit 29 : AUX interrupt
    uart_interrupt_handler();
    pop_task();
  }
}

void fake_long_handler() { wait_usec(3000000); }

void OS_enter_critical() {
  disable_interrupt();
  lock_cnt++;
#ifdef DEBUG
  if (lock_cnt > 1) {
    uart_send_string("OS_enter_critical warn: lock_cnt == ");
    uart_int(lock_cnt);
    uart_send_string("\r\n");
  }
#endif
}

void OS_exit_critical() {
  lock_cnt--;
  if (lock_cnt < 0) {
    uart_puts("OS_exit_critical error: lock_cnt < 0");
  } else if (lock_cnt == 0) {
    enable_interrupt();
  }
#ifdef DEBUG
  else {
    uart_send_string("OS_exit_critical warn: lock_cnt == ");
    uart_int(lock_cnt);
    uart_send_string("\r\n");
  }
#endif
}