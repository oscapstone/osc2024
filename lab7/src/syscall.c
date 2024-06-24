#include "syscall.h"
#include "dev_framebuffer.h"
#include "initrd.h"
#include "irq.h"
#include "mbox.h"
#include "mm.h"
#include "sched.h"
#include "signal.h"
#include "string.h"
#include "traps.h"
#include "uart.h"
#include "utils.h"
#include "vfs.h"
#include "vm.h"

extern unsigned int height;
extern unsigned int width;
extern unsigned int pitch;
extern unsigned int isrgb;

extern void child_ret_from_fork();

int sys_getpid()
{
    return get_current()->pid;
}

size_t sys_uart_read(char *buf, size_t size)
{
    int i = 0;
    while (i < size)
        buf[i++] = uart_getc();
    return i;
}

size_t sys_uart_write(const char *buf, size_t size)
{
    int i = 0;
    while (i < size)
        uart_putc(buf[i++]);
    return i;
}

int sys_exec(const char *name, char *const argv[])
{
    // TODO: Implement exec
    uart_puts("[SYS_EXEC]\n");
    while (1)
        ;
    // initrd_sys_exec(name);
    return 0;
}

int sys_fork(trap_frame *tf)
{
    disable_interrupt();
    struct task_struct *parent = get_current();
    struct task_struct *child = kthread_create(0);

    map_pages((unsigned long)child->pgd, 0x0, parent->code_size,
              VIRT_TO_PHYS((unsigned long)parent->start), 0);
    map_pages((unsigned long)child->pgd, 0xFFFFFFFFB000, 0x4000,
              (unsigned long)VIRT_TO_PHYS(child->user_stack), 0);
    map_pages((unsigned long)child->pgd, 0x3C000000, 0x3000000, 0x3C000000, 0);

    // Copy kernel stack and user stack
    memcpy(child->stack, parent->stack, STACK_SIZE);
    memcpy(child->user_stack, parent->user_stack, STACK_SIZE);

    // Copy signal handlers
    memcpy(child->sighand, parent->sighand, sizeof(parent->sighand));

    // Copy file descriptor table
    for (int i = 0; i < MAX_FD; i++) {
        if (parent->fdt[i]) {
            child->fdt[i] = kmalloc(sizeof(struct file));
            memcpy(child->fdt[i], parent->fdt[i], sizeof(struct file));
        }
    }

    unsigned long sp_off = (unsigned long)tf - (unsigned long)parent->stack;
    trap_frame *child_trap_frame = (trap_frame *)(child->stack + sp_off);

    child->context.lr = (unsigned long)child_ret_from_fork;
    child->context.sp = (unsigned long)child_trap_frame;
    child->context.fp = (unsigned long)child_trap_frame;

    child_trap_frame->sp_el0 = tf->sp_el0;
    child_trap_frame->x0 = 0;

    enable_interrupt();
    return child->pid;
}

void sys_exit()
{
    kthread_exit();
}

int sys_mbox_call(unsigned char ch, unsigned int *mbox)
{
    if (((unsigned long)mbox & 0xFFFF000000000000) == 0) {
        unsigned int *temp = kmalloc(mbox[0]);
        memcpy(temp, mbox, mbox[0]);
        int ret = mbox_call(ch, (unsigned int *)temp);
        memcpy(mbox, temp, mbox[0]);
        return ret;
    }
    return mbox_call(ch, mbox);
}

void sys_kill(int pid)
{
    kthread_stop(pid);
}

void sys_signal(int signum, void (*handler)())
{
    signal(signum, handler);
}

void sys_sigkill(int pid, int sig)
{
    kill(pid, sig);
}

void sys_sigreturn(trap_frame *regs)
{
    // Restore the sigframe
    memcpy(regs, &get_current()->sigframe, sizeof(trap_frame));
    kfree(get_current()->sig_stack);
    get_current()->siglock = 0;
    return; // Jump to the previous context (user program) after eret
}

int sys_open(const char *pathname, int flags)
{
    uart_puts("[SYS_OPEN] ");
    uart_puts(pathname);
    uart_puts(" (");
    char path[PATH_MAX] = { 0 };
    uart_puts(realpath(pathname, path));
    uart_puts(")\n");

    for (int i = 0; i < MAX_FD; i++)
        if (!get_current()->fdt[i])
            if (vfs_open(path, flags, &get_current()->fdt[i]) == 0)
                return i;
    return -1;
}

int sys_close(int fd)
{
    uart_puts("[SYS_CLOSE]\n");
    if (get_current()->fdt[fd]) {
        vfs_close(get_current()->fdt[fd]);
        get_current()->fdt[fd] = 0;
        return 0;
    }
    return -1;
}

long sys_write(int fd, const void *buf, unsigned long count)
{
    uart_puts("[SYS_WRITE]\n");
    if (get_current()->fdt[fd])
        return vfs_write(get_current()->fdt[fd], buf, count);
    return -1;
}

long sys_read(int fd, void *buf, unsigned long count)
{
    uart_puts("[SYS_READ]\n");
    if (get_current()->fdt[fd])
        return vfs_read(get_current()->fdt[fd], buf, count);
    return 0;
}

int sys_mkdir(const char *pathname, unsigned mode)
{
    uart_puts("[SYS_MKDIR] ");
    uart_puts(pathname);
    uart_puts("\n");
    // TODO: Realpath
    return vfs_mkdir(pathname);
}

int sys_mount(const char *src, const char *target, const char *filesystem,
              unsigned long flags, const void *data)
{
    uart_puts("[SYS_MOUNT]\n");
    // TODO: Realpath
    return vfs_mount(target, filesystem);
}

int sys_chdir(const char *path)
{
    uart_puts("[SYS_CHDIR] ");
    uart_puts(path);
    uart_puts("\n");

    char buf[PATH_MAX];
    realpath(path, buf);
    memset(get_current()->cwd, 0, PATH_MAX);
    strncpy(get_current()->cwd, buf, strlen(buf));

    uart_puts("cwd = ");
    uart_puts(get_current()->cwd);
    uart_puts("\n");
    return 0;
}

long sys_lseek64(int fd, long offset, int whence)
{
    uart_puts("[SYS_LSEEK64]\n");
    if (whence == SEEK_SET) {
        get_current()->fdt[fd]->f_pos = offset;
        return offset;
    }
    return -1;
}

int sys_ioctl(int fd, unsigned long request, void *info)
{
    uart_puts("[SYS_IOCTL]\n");
    if (request == 0) {
        struct framebuffer_info *fb_info = info;
        fb_info->height = height;
        fb_info->width = width;
        fb_info->pitch = pitch;
        fb_info->isrgb = isrgb;
    }
    return 0;
}

/* System Call Test */

static int getpid()
{
    int pid = -1;
    asm volatile("mov x8, 0");
    asm volatile("svc 0");
    asm volatile("mov %0, x0" : "=r"(pid));
    return pid;
}

static int fork()
{
    int ret = -1;
    asm volatile("mov x8, 4");
    asm volatile("svc 0");
    asm volatile("mov %0, x0" : "=r"(ret));
    return ret;
}

static void exit()
{
    asm volatile("mov x8, 5");
    asm volatile("svc 0");
}

void fork_test()
{
    uart_puts("Fork Test (pid = ");
    uart_hex(getpid());
    uart_puts(")\n");
    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) {
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        uart_puts("first child pid: ");
        uart_hex(getpid());
        uart_puts(", cnt: ");
        uart_hex(cnt);
        uart_puts(", ptr: ");
        uart_hex((unsigned long long)&cnt);
        uart_puts(", sp: ");
        uart_hex(cur_sp);
        uart_puts("\n");
        cnt++;
        if ((ret = fork()) != 0) {
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            uart_puts("first child pid: ");
            uart_hex(getpid());
            uart_puts(", cnt: ");
            uart_hex(cnt);
            uart_puts(", ptr: ");
            uart_hex((unsigned long long)&cnt);
            uart_puts(", sp: ");
            uart_hex(cur_sp);
            uart_puts("\n");
        } else {
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                uart_puts("second child pid: ");
                uart_hex(getpid());
                uart_puts(", cnt: ");
                uart_hex(cnt);
                uart_puts(", ptr: ");
                uart_hex((unsigned long long)&cnt);
                uart_puts(", sp: ");
                uart_hex(cur_sp);
                uart_puts("\n");
                for (int i = 0; i < 1000000; i++)
                    ;
                cnt++;
            }
        }
        exit();
    } else {
        uart_puts("parent here, pid ");
        uart_hex(getpid());
        uart_puts(", child ");
        uart_hex(ret);
        uart_puts("\n");
    }
    exit();
}

void run_fork_test()
{
    // FIXME: Unable to run when virtual memory is enabled
    struct task_struct *current = get_current();
    asm volatile("msr spsr_el1, %0" ::"r"(0x340));
    asm volatile("msr elr_el1, %0" ::"r"(fork_test));
    asm volatile("msr sp_el0, %0" ::"r"(current->user_stack + STACK_SIZE));
    asm volatile("mov sp, %0" ::"r"(current->stack + STACK_SIZE));
    asm volatile("eret");
}
