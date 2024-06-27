#include "traps.h"

#include <stdint.h>

#include "irq.h"
#include "mbox.h"
#include "mem.h"
#include "scheduler.h"
#include "syscalls.h"
#include "uart.h"
#include "virtm.h"

extern thread_struct *running_tcircle;
extern uintptr_t lfb;

static void print_registers(uint64_t elr, uint64_t esr, uint64_t spsr,
                            uint64_t ttbr0_el1, uint64_t ttbr1_el1,
                            uint64_t far_el1) {
  // Print spsr_el1
  uart_log(INFO, "spsr_el1: ");
  uart_hex(spsr);
  uart_putc(NEWLINE);

  // Print elr_el1
  uart_log(INFO, "elr_el1: ");
  uart_hex(elr);
  uart_putc(NEWLINE);

  // Print esr_el1
  uart_log(INFO, "esr_el1: ");
  uart_hex(esr);
  uart_putc(NEWLINE);

  // Print ttbr0_el1
  uart_log(INFO, "ttbr0_el1: ");
  uart_hex(ttbr0_el1);
  uart_putc(NEWLINE);

  // Print ttbr1_el1
  uart_log(INFO, "ttbr1_el1: ");
  uart_hex(ttbr1_el1);
  uart_putc(NEWLINE);

  // Print far_el1
  uart_log(INFO, "far_el1: ");
  uart_hex(far_el1);
  uart_putc(NEWLINE);
}

void exception_entry(trap_frame *tf) {
  unsigned long elr, esr, spsr, ttbr0_el1, ttbr1_el1, far_el1;
  asm volatile("mrs %0, elr_el1\n"
               "mrs %1, esr_el1\n"
               "mrs %2, spsr_el1\n"
               "mrs %3, ttbr0_el1\n"
               "mrs %4, ttbr1_el1\n"
               "mrs %5, far_el1\n"
               : "=r"(elr),       //
                 "=r"(esr),       //
                 "=r"(spsr),      //
                 "=r"(ttbr0_el1), //
                 "=r"(ttbr1_el1), //
                 "=r"(far_el1)    //
               :
               : "memory");
  if (esr != 0x56000000) {
    print_registers(elr, esr, spsr, ttbr0_el1, ttbr1_el1, far_el1);
    while (1)
      ;
  }

  enable_interrupt(); // Prevent uart blocking the interrupt[?]

  unsigned int syscall_num = tf->x8;
  switch (syscall_num) {
  case 0: { // int getpid()
    unsigned int test = tf->x7;
    unsigned int pid = sys_getpid();
    tf->x0 = pid;
    if (test != 255) {
      list_tcircle();
      list_fd(get_current());
    }
    break;
  }
  case 1: { // size_t uartread(char buf[], size_t size)
    char *buf = (char *)tf->x0;
    size_t size = tf->x1;
    tf->x0 = sys_uart_read(buf, size);
    break;
  }
  case 2: { // size_t uartwrite(const char buf[], size_t size)
    char *buf = (char *)tf->x0;
    size_t size = tf->x1;
    tf->x0 = sys_uart_write(buf, size);
    break;
  }
  case 3: { // int exec(const char *name, char *const argv[])
    const char *name = (char *)tf->x0;
    tf->x0 = sys_exec(name, tf);
    tf->elr_el1 = 0x0;
    tf->sp_el0 = USER_STACK + STACK_SIZE;
    break;
  }
  case 4: { // int fork()
    tf->x0 = sys_fork(tf);
    list_tcircle();
    break;
  }
  case 5: { // void exit(int status)
    int status = tf->x0;
    sys_exit(status);
    break;
  }
  case 6: { // int mbox_call(unsigned char ch, unsigned int *mbox)
    unsigned char ch = tf->x0;
    unsigned int *mbox = (unsigned int *)tf->x1;
    tf->x0 = sys_mbox_call(ch, mbox);
    break;
  }
  case 7: { // void kill(int pid)
    unsigned int pid = tf->x0;
    kill_thread_by_pid(pid);
    break;
  }
  case 8: { // signal(int SIGNAL, void (*handler)())
    unsigned int SIGNAL = tf->x0;
    void (*handler)() = (void (*)())tf->x1;
    signal(SIGNAL, handler);
    break;
  }
  case 9: { // (signal_)kill(int pid, int SIGNAL)
    unsigned int pid = tf->x0;
    unsigned int SIGNAL = tf->x1;
    signal_kill(pid, SIGNAL);
    break;
  }
  case 11: { // int open(const char *pathname, int flags)
    const char *pathname = (char *)tf->x0;
    int flags = tf->x1;
    tf->x0 = sys_open(pathname, flags);
    break;
  }
  case 12: { // int close(int fd)
    int fd = tf->x0;
    tf->x0 = sys_close(fd);
    break;
  }
  case 13: { // int write(int fd, void *buf, size_t count)
    int fd = tf->x0;
    void *buf = (void *)tf->x1;
    size_t count = tf->x2;
    tf->x0 = sys_write(fd, buf, count);
    break;
  }
  case 14: { // int read(int fd, void *buf, size_t count)
    int fd = tf->x0;
    void *buf = (void *)tf->x1;
    size_t count = tf->x2;
    tf->x0 = sys_read(fd, buf, count);
    break;
  }
  case 15: { // int mkdir(const char *pathname, unsigned mode)
    const char *pathname = (char *)tf->x0;
    unsigned mode = tf->x1;
    tf->x0 = sys_mkdir(pathname, mode);
    break;
  }
  case 16: { // int mount(const char *src, const char *target,
             //          const char *filesystem, unsigned long flags,
             //          const void *data)
    // const char *src = (char *)tf->x0;
    const char *target = (char *)tf->x1;
    const char *fs = (char *)tf->x2;
    tf->x0 = sys_mount(target, fs); //, flags, data);
    break;
  }
  case 17: { // int chdir(const char *path)
    const char *path = (char *)tf->x0;
    tf->x0 = sys_chdir(path);
    break;
  }
  case 18: { // long lseek(int fd, off_t offset, int whence)
    int fd = tf->x0;
    long offset = tf->x1;
    int whence = tf->x2;
    tf->x0 = sys_lseek64(fd, offset, whence);
    break;
  }
  case 19: { // int ioctl(int fd, unsigned long request, ...)
    int fd = tf->x0;
    unsigned long request = tf->x1;
    void *info = (void *)tf->x2;
    tf->x0 = sys_ioctl(fd, request, info);
    if (request == GET_INFO) {
      uart_log(INFO, "GET_INFO lfb = ");
      uart_hex(lfb);
      uart_putc(NEWLINE);
    }
    break;
  }
  case 139: {
    sys_sigreturn(tf);
    break;
  }
  default:
    uart_log(ERR, "Invalid system call ");
    uart_dec(syscall_num);
    uart_putc(NEWLINE);
  }
}

void invalid_entry(uint64_t elr, uint64_t esr, uint64_t spsr,
                   uint64_t ttbr0_el1, uint64_t ttbr1_el1, uint64_t far_el1) {
  uart_log(ERR, "The exception handler is not implemented\n");
  print_registers(elr, esr, spsr, ttbr0_el1, ttbr1_el1, far_el1);

  if ((esr & 0xFC000000) == 0x94000000) {
    uint32_t svc_number = esr & 0xFFFF;
    uart_log(ERR, "SVC Call Exception. SVC Number: ");
    uart_dec(svc_number);
    uart_putc(NEWLINE);
  } else {
    uart_log(ERR, "Unknown Exception\n");
  }
  while (1)
    ;
}