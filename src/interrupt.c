#include "interrupt.h"

#include "multitask.h"
#include "syscall_.h"
#include "task.h"
#include "timer.h"
#include "uart1.h"
#include "utli.h"

int32_t lock_cnt = 0;

void check_check() { uart_puts("check_check"); }

void invaild_exception_handler() {
  uart_puts("invaild exception handler!");
  uart_puts("Rebooting...");
  reset(1000);
  while (1);
}

void el1h_sync_handler() {
  enable_interrupt();
  OS_enter_critical();

  uint32_t esr_el1_val, EC_val;
  asm volatile("mrs %0, esr_el1" : "=r"(esr_el1_val));
  EC_val = (esr_el1_val & EC_MASK) >> EC_shift;
  if (EC_val == EXCEPTION_DATA_ABORT_FR_SAME ||
      EC_val == EXCEPTION_INSTR_ABORT_FR_SAME) {
    uint32_t DFSC = (esr_el1_val & DFSC_MASK);
    uint64_t fault_addr;
    asm volatile("mrs %0, far_el1" : "=r"(fault_addr));

    if (DFSC == TASNSLATION_FAULT_L0 || DFSC == TASNSLATION_FAULT_L1 ||
        DFSC == TASNSLATION_FAULT_L2 || DFSC == TASNSLATION_FAULT_L3) {
      translation_fault_handler(fault_addr);
    } else {
      invaild_exception_handler();
    }
  } else {
    invaild_exception_handler();
  }
  OS_exit_critical();
}
extern int32_t lock_cnt;
void el0_64_sync_handler(trapframe_t *tpf) {
  enable_interrupt();
  OS_enter_critical();

  uint32_t esr_el1_val, EC_val;

  asm volatile("mrs %0, esr_el1" : "=r"(esr_el1_val));
  EC_val = (esr_el1_val & EC_MASK) >> EC_shift;

  if (EC_val == EXCEPTION_SVC) {
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
      case SYSCALL_MMAP:
        sys_mmap(tpf);
        break;
      case SYSCALL_OPEN:
        sys_open(tpf);
        break;
      case SYSCALL_CLOSE:
        sys_close(tpf);
        break;
      case SYSCALL_WRITE:
        sys_write(tpf);
        break;
      case SYSCALL_READ:
        sys_read(tpf);
        break;
      case SYSCALL_MKDIR:
        sys_mkdir(tpf);
        break;
      case SYSCALL_MOUNT:
        sys_mount(tpf);
        break;
      case SYSCALL_CHDIR:
        sys_chdir(tpf);
        break;
      case SYSCALL_LSEEK64:
        sys_lseek64(tpf);
        break;
      case SYSCALL_SYNCFS:
        sys_syncfs(tpf);
        break;
      case SIG_RETURN:
        sig_return();
        break;
      default:
        uart_send_string("err: undefiend syscall number: ");
        uart_int(syscall_num);
        uart_send_string("\r\n");
        break;
    }
  } else if (EC_val == EXCEPTION_DATA_ABORT_FR_LOW ||
             EC_val == EXCEPTION_INSTR_ABORT_FR_LOW) {
    uint32_t DFSC = (esr_el1_val & DFSC_MASK);
    uint64_t fault_addr;
    asm volatile("mrs %0, far_el1" : "=r"(fault_addr));

    if (DFSC == TASNSLATION_FAULT_L0 || DFSC == TASNSLATION_FAULT_L1 ||
        DFSC == TASNSLATION_FAULT_L2 || DFSC == TASNSLATION_FAULT_L3) {
      translation_fault_handler(fault_addr);
    } else if (DFSC == PERMISSION_FAULT_L0 || DFSC == PERMISSION_FAULT_L1 ||
               DFSC == PERMISSION_FAULT_L2 || DFSC == PERMISSION_FAULT_L3) {
      permission_fault_handler(fault_addr);
    } else {
      invaild_exception_handler();
    }

  } else {
    uart_send_string("EC_val: ");
    uart_hex(EC_val);
    uart_send_string("\r\n");
    invaild_exception_handler();
  }
  OS_exit_critical();
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