#include "syscall_.h"

#include "cpio_.h"
#include "interrupt.h"
#include "mbox.h"
#include "mem.h"
#include "multitask.h"
#include "peripherals/mbox.h"
#include "string.h"
#include "uart1.h"
#include "utli.h"
#include "vm.h"

extern task_struct* threads;
extern volatile uint32_t __attribute__((aligned(16))) mbox[36];

uint64_t sys_getpid(trapframe_t* tpf) {
  tpf->x[0] = current_thread->pid;
  return tpf->x[0];
}

uint32_t sys_uartread(trapframe_t* tpf, char* buf, uint32_t size) {
  uint32_t i;
  for (i = 0; i < size; i++) {
    buf[i] = uart_read();
  }
  tpf->x[0] = i;
  return i;
}

uint32_t sys_uartwrite(trapframe_t* tpf, const char* buf, uint32_t size) {
  uint32_t i;
  for (i = 0; i < size; i++) {
    uart_write(buf[i]);
  }
  tpf->x[0] = i;
  return i;
}

uint32_t exec(trapframe_t* tpf, const char* name, char* const argv[]) {
  uint32_t file_sz;
  char* prog = cpio_load(name, &file_sz);
  if (!prog) {
    tpf->x[0] = -1;
    return -1;
  }

  OS_enter_critical();
  vm_free(current_thread);

  current_thread->data_size = file_sz;
  current_thread->data = malloc(file_sz);
  map_pages(current_thread, CODE, (uint64_t)current_thread->data, USR_CODE_ADDR,
            file_sz, VM_PROT_READ | VM_PROT_EXEC, MAP_ANONYMOUS);
  memcpy(current_thread->data, prog, file_sz);

  current_thread->usr_stk = malloc(USR_STK_SZ);
  map_pages(current_thread, STACK, (uint64_t)current_thread->usr_stk,
            USR_STK_ADDR, USR_STK_SZ, VM_PROT_READ | VM_PROT_WRITE,
            MAP_ANONYMOUS);
  map_pages(current_thread, IO, phy2vir(IO_PM_START_ADDR), IO_PM_START_ADDR,
            IO_PM_END_ADDR - IO_PM_START_ADDR, VM_PROT_READ | VM_PROT_WRITE,
            MAP_ANONYMOUS);

  memset(current_thread->sig_handlers, 0, sizeof(current_thread->sig_handlers));
  current_thread->sig_handlers[SYSCALL_SIGKILL].func = sig_kill_default_handler;

  tpf->elr_el1 = USR_CODE_ADDR;
  tpf->sp_el0 = USR_STK_ADDR + USR_STK_SZ;
  OS_exit_critical();

  tpf->x[0] = 0;
  return 0;
}

uint32_t fork(trapframe_t* tpf) {
  task_struct* child = get_free_thread();
  if (!child) {
    uart_puts("new process allocation fail");
    tpf->x[0] = -1;
    return -1;
  }

  task_struct* parent = current_thread;

  OS_enter_critical();
  // kernel space
  child->ker_stk = malloc(KER_STK_SZ);
  memcpy(child->ker_stk, parent->ker_stk, KER_STK_SZ);
  for (int i = 0; i < SIG_NUM; i++) {
    child->sig_handlers[i].func = parent->sig_handlers[i].func;
    child->sig_handlers[i].registered = parent->sig_handlers[i].registered;
  }

  // user space
  map_pages(child, IO, phy2vir(IO_PM_START_ADDR), IO_PM_START_ADDR,
            IO_PM_END_ADDR - IO_PM_START_ADDR, VM_PROT_READ | VM_PROT_WRITE,
            MAP_ANONYMOUS);
  child->data_size = parent->data_size;
  for (vm_area_struct* ptr = parent->mm.mmap_list_head;
       ptr != (vm_area_struct*)0; ptr = ptr->vm_next) {
    if (ptr->vm_type != IO) {
      map_pages(child, ptr->vm_type, phy2vir(ptr->pm_start), ptr->vm_start,
                ptr->area_sz, VM_PROT_READ, ptr->vm_flag);
    }
  }

  // change_all_page_prot((uint64_t*)phy2vir(parent->mm.pgd), 0, VM_PROT_READ);
  // copy_all_pt((uint64_t*)phy2vir(child->mm.pgd),
  //             (uint64_t*)phy2vir(parent->mm.pgd), 0);

  OS_exit_critical();
#ifdef DEBUG
  uart_send_string("parent->ker_stk: ");
  uart_hex_64((uint64_t)parent->ker_stk);
  uart_send_string(", child->ker_stk: ");
  uart_hex_64((uint64_t)child->ker_stk);
  uart_send_string("\r\n");
  uart_send_string("parent->usr_stk: ");
  uart_hex_64((uint64_t)parent->usr_stk);
  uart_send_string(", child->usr_stk: ");
  uart_hex_64((uint64_t)child->usr_stk);
  uart_send_string("\r\n");
  uart_send_string("parent->data: ");
  uart_hex_64((uint64_t)parent->data);
  uart_send_string(", child->data: ");
  uart_hex_64((uint64_t)child->data);
  uart_send_string("\r\n");
#endif

  store_cpu_context(&parent->cpu_context);
  if (current_thread->pid == parent->pid) {
    memcpy(&child->cpu_context, &parent->cpu_context, sizeof(cpu_context_t));
    child->cpu_context.sp =
        (uint64_t)child->ker_stk +
        (parent->cpu_context.sp - (uint64_t)parent->ker_stk);
    child->cpu_context.fp =
        (uint64_t)child->ker_stk +
        (parent->cpu_context.fp - (uint64_t)parent->ker_stk);
#ifdef DEBUG
    uart_send_string("parent->cpu_context.sp: ");
    uart_hex_64(parent->cpu_context.sp);
    uart_send_string(", child->cpu_context.sp: ");
    uart_hex_64(child->cpu_context.sp);
    uart_send_string("\r\n");
    uart_send_string("parent->cpu_context.fp: ");
    uart_hex_64(parent->cpu_context.fp);
    uart_send_string(", child->cpu_context.fp: ");
    uart_hex_64(child->cpu_context.fp);
    uart_send_string("\r\n");
    uart_send_string("parent->cpu_context.lr: ");
    uart_hex_64(parent->cpu_context.lr);
    uart_send_string(", child->cpu_context.lr: ");
    uart_hex_64(child->cpu_context.lr);
    uart_send_string("\r\n");
#endif

    tpf->x[0] = child->pid;
    ready_que_push_back(child);
    return child->pid;
  }
  trapframe_t* child_tpf =
      (trapframe_t*)(child->ker_stk +
                     ((uint64_t)tpf - (uint64_t)parent->ker_stk));
  child_tpf->sp_el0 = tpf->sp_el0;
  child_tpf->elr_el1 = tpf->elr_el1;
#ifdef DEBUG
  uart_send_string("parent_tpf: ");
  uart_hex_64((uint64_t)tpf);
  uart_send_string(", child_tpf: ");
  uart_hex_64((uint64_t)child_tpf);
  uart_send_string("\r\n");
  uart_send_string("parent_tpf->sp_el0: ");
  uart_hex_64(tpf->sp_el0);
  uart_send_string(", child_tpf->sp_el0 : ");
  uart_hex_64(child_tpf->sp_el0);
  uart_send_string("\r\n");
  uart_send_string("parent_tpf->elr_el1: ");
  uart_hex_64(tpf->elr_el1);
  uart_send_string(", child_tpf->elr_el1: ");
  uart_hex_64(child_tpf->elr_el1);
  uart_send_string("\r\n");
#endif
  child_tpf->x[0] = 0;
  return 0;
}

void exit() { task_exit(); }

uint32_t sys_mbox_call(trapframe_t* tpf, uint8_t ch, uint32_t* usr_mbox) {
  uint32_t mbox_sz = usr_mbox[0];
  OS_enter_critical();
  memcpy((void*)mbox, (void*)usr_mbox, mbox_sz);
  tpf->x[0] = mbox_call(ch);
  memcpy((void*)usr_mbox, (void*)mbox, mbox_sz);
  OS_exit_critical();
  return 0;
}

void kill(uint32_t pid) {
  if (pid <= 0 || pid > PROC_NUM || threads[pid - 1].status == THREAD_FREE) {
    uart_send_string("pid=");
    uart_int(pid);
    uart_puts(" not exists");
    return;
  }

  OS_enter_critical();
  threads[pid - 1].status = THREAD_DEAD;
  ready_que_del_node(&threads[pid - 1]);
  OS_exit_critical();
}

void signal(uint32_t SIGNAL, sig_handler_func handler) {
  if (SIGNAL < 1 || SIGNAL >= SIG_NUM) {
    uart_puts("unvaild signal number");
    return;
  }
  OS_enter_critical();
  current_thread->sig_handlers[SIGNAL].registered = 1;
  current_thread->sig_handlers[SIGNAL].func = handler;
  OS_exit_critical();
}

void sig_kill(uint32_t pid, uint32_t SIGNAL) {
  if (pid <= 0 || pid > PROC_NUM || threads[pid - 1].status == THREAD_FREE) {
    uart_send_string("pid=");
    uart_int(pid);
    uart_puts(" not exists");
    return;
  }
  if (SIGNAL < 1 || SIGNAL >= SIG_NUM) {
    uart_send_string("SIGNAL=");
    uart_int(SIGNAL);
    uart_puts(" not exists");
  }
  OS_enter_critical();
  threads[pid - 1].sig_handlers[SIGNAL].sig_cnt++;
  OS_exit_critical();
}

void sys_mmap(trapframe_t* tpf) {
  uint64_t addr = (uint64_t)tpf->x[0];
  uint32_t len = (uint32_t)tpf->x[1];
  uint8_t prot = (uint8_t)tpf->x[2];
  uint8_t flags = (uint8_t)tpf->x[3];

  uart_send_string("addr: ");
  uart_hex_64(addr);
  uart_send_string(", len: ");
  uart_int(len);
  uart_send_string(", prot: ");
  uart_hex(prot);
  uart_send_string(", flags: ");
  uart_hex(flags);
  uart_send_string("\r\n");

  if (addr) {
    for (vm_area_struct* ptr = current_thread->mm.mmap_list_head;
         ptr != (vm_area_struct*)0; ptr = ptr->vm_next) {
      if (ptr->vm_start <= addr && ptr->vm_start + ptr->area_sz > addr) {
        addr = 0;
        break;
      }
    }
  }

  if (!addr) {
    for (;;) {  // To find a free VMA region
      uint8_t used = 0;
      for (vm_area_struct* ptr = current_thread->mm.mmap_list_head;
           ptr != (vm_area_struct*)0; ptr = ptr->vm_next) {
        if (ptr->vm_start <= addr && ptr->vm_start + ptr->area_sz > addr) {
          used = 1;
          break;
        }
      }
      if (used) {
        addr += 0x1000;
      } else {
        break;
      }
    }
  }

  void* ker_addr = malloc(len);
  map_pages(current_thread, DATA, (uint64_t)ker_addr, addr, len, prot, flags);
  tpf->x[0] = addr;
}

static void exec_signal_handler() {
  current_thread->cur_exec_sig_func();
  asm volatile(
      "mov x8, 32\n\t"
      "svc 0\n\t");
}

void check_signal() {
  OS_enter_critical();
  if (current_thread->sig_is_check == 1) {
    OS_exit_critical();
    return;
  }
  current_thread->sig_is_check = 1;
  OS_exit_critical();

  for (int i = 1; i < SIG_NUM; i++) {
    if (current_thread->sig_handlers[i].registered == 0) {
      while (current_thread->sig_handlers[i].sig_cnt > 0) {
        OS_enter_critical();
        current_thread->sig_handlers[i].sig_cnt--;
        OS_exit_critical();
        current_thread->sig_handlers[i].func();
      }
    } else {
      // volatile int check = 0;
      store_cpu_context(&current_thread->sig_cpu_context);
      // if (check == 1) {
      //   uart_puts("cpu context restored");
      // }
      if (current_thread->sig_handlers[i].sig_cnt > 0) {
        // check = 1;
        OS_enter_critical();
        current_thread->sig_handlers[i].sig_cnt--;
        current_thread->cur_exec_sig_func =
            current_thread->sig_handlers[i].func;
        current_thread->sig_stk = malloc(USR_SIG_STK_SZ);
        map_pages(current_thread, STACK, (uint64_t)current_thread->sig_stk,
                  USR_SIG_STK_ADDR, USR_SIG_STK_SZ,
                  VM_PROT_READ | VM_PROT_WRITE, MAP_ANONYMOUS);
        OS_exit_critical();
        exec_in_el0(exec_signal_handler,
                    (void*)(USR_SIG_STK_ADDR + USR_SIG_STK_SZ));
      }
    }
  }
  current_thread->sig_is_check = 0;
}

void sig_kill_default_handler() { task_exit(); }

void sig_return() {
  current_thread->cur_exec_sig_func = (sig_handler_func)0;
  free_pages(current_thread, (uint64_t)current_thread->sig_stk);
  current_thread->sig_stk = (void*)0;
  load_cpu_context(&current_thread->sig_cpu_context);
}