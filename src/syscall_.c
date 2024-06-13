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
#include "vfs.h"
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
    OS_exit_critical();
    buf[i] = uart_read();
    OS_enter_critical();
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
  file* fp = vfs_open(name, 0);
  if (!fp) {
    tpf->x[0] = -1;
    return -1;
  }

  vm_free(current_thread);

  current_thread->usr_stk = malloc(USR_STK_SZ);
  map_pages(current_thread, STACK, (uint64_t)current_thread->usr_stk,
            USR_STK_ADDR, USR_STK_SZ, VM_PROT_READ | VM_PROT_WRITE,
            MAP_ANONYMOUS);

  map_pages(current_thread, IO, phy2vir(IO_PM_START_ADDR), IO_PM_START_ADDR,
            IO_PM_END_ADDR - IO_PM_START_ADDR, VM_PROT_READ | VM_PROT_WRITE,
            MAP_ANONYMOUS);

  initramfs_internal* internal = (initramfs_internal*)fp->vnode->internal;
  uint32_t file_sz = internal->file_sz;
  current_thread->data_size = file_sz;
  current_thread->data = malloc(file_sz);
  memcpy(current_thread->data, internal->data, file_sz);
  map_pages(current_thread, CODE, (uint64_t)current_thread->data, USR_CODE_ADDR,
            file_sz, VM_PROT_READ | VM_PROT_EXEC, MAP_ANONYMOUS);

  memset(current_thread->sig_handlers, 0, sizeof(current_thread->sig_handlers));
  current_thread->sig_handlers[SYSCALL_SIGKILL].func = sig_kill_default_handler;

  vfs_close(fp);

  tpf->elr_el1 = USR_CODE_ADDR;
  tpf->sp_el0 = USR_STK_ADDR + USR_STK_SZ;
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
  OS_enter_critical();
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
  memcpy((void*)mbox, (void*)usr_mbox, mbox_sz);
  tpf->x[0] = mbox_call(ch);
  memcpy((void*)usr_mbox, (void*)mbox, mbox_sz);
  return 0;
}

void kill(uint32_t pid) {
  if (pid <= 0 || pid > PROC_NUM || threads[pid - 1].status == THREAD_FREE) {
    uart_send_string("pid=");
    uart_int(pid);
    uart_puts(" not exists");
    return;
  }

  threads[pid - 1].status = THREAD_DEAD;
  ready_que_del_node(&threads[pid - 1]);
}

void signal(uint32_t SIGNAL, sig_handler_func handler) {
  if (SIGNAL < 1 || SIGNAL >= SIG_NUM) {
    uart_puts("unvaild signal number");
    return;
  }
  current_thread->sig_handlers[SIGNAL].registered = 1;
  current_thread->sig_handlers[SIGNAL].func = handler;
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

  threads[pid - 1].sig_handlers[SIGNAL].sig_cnt++;
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
  if (current_thread->sig_is_check == 1) {
    return;
  }
  current_thread->sig_is_check = 1;

  for (int i = 1; i < SIG_NUM; i++) {
    if (current_thread->sig_handlers[i].registered == 0) {
      while (current_thread->sig_handlers[i].sig_cnt > 0) {
        current_thread->sig_handlers[i].sig_cnt--;
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
        current_thread->sig_handlers[i].sig_cnt--;
        current_thread->cur_exec_sig_func =
            current_thread->sig_handlers[i].func;
        current_thread->sig_stk = malloc(USR_SIG_STK_SZ);
        map_pages(current_thread, STACK, (uint64_t)current_thread->sig_stk,
                  USR_SIG_STK_ADDR, USR_SIG_STK_SZ,
                  VM_PROT_READ | VM_PROT_WRITE, MAP_ANONYMOUS);
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

void sys_open(trapframe_t* tpf) {
  const char* pathname = (const char*)tpf->x[0];
  uint8_t flags = (uint8_t)tpf->x[1];

  file* fp = vfs_open(pathname, flags);
  if (!fp) {
    tpf->x[0] = -1;
    return;
  }

  int32_t fd = -1;
  file** fdtable = current_thread->fdtable;
  for (uint32_t i = 0; i < MAX_FD; i++) {
    if (!current_thread->fdtable[i]) {
      fdtable[i] = fp;
      fd = i;
      break;
    }
  }

  tpf->x[0] = fd;
}

void sys_close(trapframe_t* tpf) {
  int32_t fd = tpf->x[0];
  file* fp = current_thread->fdtable[fd];
  tpf->x[0] = vfs_close(fp);
  current_thread->fdtable[fd] = (file*)0;
}

void sys_write(trapframe_t* tpf) {
  int32_t fd = tpf->x[0];
  const void* buf = (const void*)tpf->x[1];
  uint32_t len = tpf->x[2];
  file* fp = current_thread->fdtable[fd];
  if (!fp) {
    tpf->x[0] = -1;
  } else {
    tpf->x[0] = vfs_write(fp, buf, len);
  }
}

void sys_read(trapframe_t* tpf) {
  int32_t fd = tpf->x[0];
  void* buf = (void*)tpf->x[1];
  uint32_t len = tpf->x[2];
  file* fp = current_thread->fdtable[fd];
  if (!fp) {
    tpf->x[0] = -1;
  } else {
    tpf->x[0] = vfs_read(fp, buf, len);
  }
}

//  ignore mode, since there is no access control
void sys_mkdir(trapframe_t* tpf) {
  const char* pathname = (const char*)tpf->x[0];
  tpf->x[0] = vfs_mkdir(pathname);
}

// ignore arguments other than target (where to mount) and filesystem (fs name)
void sys_mount(trapframe_t* tpf) {
  const char* mountpoint = (const char*)tpf->x[1];
  const char* fs = (const char*)tpf->x[2];
  tpf->x[0] = vfs_mount(mountpoint, fs);
}

void sys_chdir(trapframe_t* tpf) {
  const char* pathname = (const char*)tpf->x[0];
  tpf->x[0] = vfs_chdir(pathname);
}

// only need to implement seek set
void sys_lseek64(trapframe_t* tpf) {
  int32_t fd = tpf->x[0];
  int64_t offset = tpf->x[1];
  int32_t whence = tpf->x[2];
  file* fp = current_thread->fdtable[fd];
  if (!fp) {
    tpf->x[0] = -1;
  } else {
    tpf->x[0] = vfs_lseek64(fp, offset, whence);
  }
}
