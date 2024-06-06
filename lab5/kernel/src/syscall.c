#include <kernel/bsp_port/irq.h>
#include <kernel/bsp_port/mailbox.h>
#include <kernel/bsp_port/ramfs.h>
#include <kernel/bsp_port/syscall.h>
#include <kernel/bsp_port/uart.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/sched.h>
#include <kernel/signal.h>
#include <bsp/asm/traps.h>
#include <lib/string.h>
#include <lib/utils.h>

extern void child_ret_from_fork();

int sys_getpid() { return get_current()->pid; }

size_t sys_uart_read(char *buf, size_t size) {
    int i = 0;
    while (i < size) buf[i++] = uart_getc();
    return i;
}

size_t sys_uart_write(const char *buf, size_t size) {
    int i = 0;
    while (i < size) uart_send(buf[i++]);
    return i;
}

int sys_exec(const char *file_name, char *const argv[]) {
    unsigned long file_size = ramfs_get_file_size(file_name);
    char *file_contents = kmalloc(file_size);
    ramfs_get_file_contents(file_name, file_contents);

    if (file_contents == NULL) {
        print_string("[ERROR] File not found: ");
        print_string(file_name);
        print_string("\n");
        return 0;
    }
    print_string("\n[INFO] Executing file: ");
    print_string(file_name);
    print_string("\n");

    char *user_program = kmalloc(file_size);
    get_current()->state = TASK_STOPPED;
    memcpy(user_program, file_contents, file_size);
    get_current()->sigpending = 0;
    memset(get_current()->sighand, 0, sizeof(get_current()->sighand));
    get_current()->context.lr = (unsigned long)user_program;
    return 0;
}

int sys_fork(trap_frame *tf) {
    disable_irq();
    struct task_struct *parent = get_current();
    struct task_struct *child = kthread_create(0);

    // child->parent_pid = parent->pid;
    // parent->child_count++;
    // Copy the parent's memory
    memcpy(child->stack, parent->stack, STACK_SIZE);
    memcpy(child->user_stack, parent->user_stack, STACK_SIZE);
    memcpy(child->sighand, parent->sighand, sizeof(parent->sighand));

    unsigned long sp_off = (unsigned long)tf - (unsigned long)parent->stack;
    trap_frame *child_trap_frame = (trap_frame *)(child->stack + sp_off);

    child->context.sp = (unsigned long)child_trap_frame;
    child->context.lr = (unsigned long)child_ret_from_fork;

    unsigned long sp_el0_off = tf->sp_el0 - (unsigned long)parent->user_stack;
    child_trap_frame->sp_el0 = (unsigned long)child->user_stack + sp_el0_off;
    child_trap_frame->x0 = 0;

    enable_irq();
    return child->pid;
}

void sys_exit() { kthread_exit(); }

int sys_mbox_call(unsigned char ch, unsigned int *mbox) {
    return mailbox_call(ch, mbox);
}

void sys_kill(int pid) { kthread_stop(pid); }

void sys_signal(int signum, void (*handler)()) { signal(signum, handler); }

void sys_sigkill(int pid, int sig) { sigkill(pid, sig); }

void sys_sigreturn(trap_frame *regs) {
    // Restore the sigframe
    memcpy(regs, &get_current()->sigframe, sizeof(trap_frame));
    kfree(get_current()->sig_stack);
    get_current()->sighandling = 0;
    return;  // Jump to the previous context (user program) after eret
}

// emit the syscall instruction
static int getpid() {
    long pid = -1;
    asm volatile("mov x8, 0");
    asm volatile("svc 0");
    asm volatile("mov %0, x0" : "=r"(pid));
    return pid;
}

static int fork() {
    long ret = -1;
    asm volatile("mov x8, 4");
    asm volatile("svc 0");
    asm volatile("mov %0, x0" : "=r"(ret));
    return ret;
}

static void exit() {
    asm volatile("mov x8, 5");
    asm volatile("svc 0");
}

void fork_test() {
    print_string("Fork Test (pid = ");
    print_d(getpid());
    print_string(")\n");
    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) {
        print_string("first child pid: ");
        print_d(getpid());
        print_string(", cnt: ");
        print_d(cnt);
        print_string(", ptr: ");
        print_h((unsigned long long)&cnt);
        print_string("\n");
        cnt++;
        if ((ret = fork()) != 0) {
            print_string("first child pid: ");
            print_d(getpid());
            print_string(", cnt: ");
            print_d(cnt);
            print_string(", ptr: ");
            print_h((unsigned long long)&cnt);
            print_string("\n");
        } else {
            while (cnt < 5) {
                print_string("second child pid: ");
                print_d(getpid());
                print_string(", cnt: ");
                print_d(cnt);
                print_string(", ptr: ");
                print_h((unsigned long long)&cnt);
                print_string("\n");
                for (int i = 0; i < 1000000; i++);
                cnt++;
            }
        }
        exit();
    } else {
        print_string("parent here, pid ");
        print_d(getpid());
        print_string(", child ");
        print_d(ret);
        print_string("\n");
    }
    exit();
}

void run_fork_test() {
    asm volatile("msr spsr_el1, %0" ::"r"(0x0));
    asm volatile("msr elr_el1, %0" ::"r"(fork_test));
    asm volatile("msr sp_el0, %0" ::"r"(get_current()->context.sp));
    asm volatile("mov sp, %0" ::"r"(get_current()->stack + STACK_SIZE));
    asm volatile("eret");
}
