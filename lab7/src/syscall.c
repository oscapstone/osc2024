#include "syscall.h"
#include "cpio.h"
#include "dev_framebuffer.h"
#include "exception.h"
#include "mbox.h"
#include "memory.h"
#include "mmu.h"
#include "sched.h"
#include "signal.h"
#include "stddef.h"
#include "string.h"
#include "uart1.h"
#include "vfs.h"

int getpid(trapframe_t *tpf)
{
    tpf->x0 = curr_thread->pid;
    return curr_thread->pid;
}

size_t uartread(trapframe_t *tpf, char buf[], size_t size)
{
    int i = 0;
    for (int i = 0; i < size; i++) {
        buf[i] = uart_async_getc();
    }
    tpf->x0 = i;
    return i;
}

size_t uartwrite(trapframe_t *tpf, const char buf[], size_t size)
{
    int i = 0;
    for (int i = 0; i < size; i++) {
        uart_async_putc(buf[i]);
    }
    tpf->x0 = i;
    return i;
}

// In this lab, you won’t have to deal with argument passing
int exec(trapframe_t *tpf, const char *name, char *const argv[])
{
    kfree(curr_thread->data);

    curr_thread->datasize = get_file_size((char *)name);
    char *new_data = get_file_start((char *)name);
    curr_thread->data = kmalloc(curr_thread->datasize);

    for (unsigned int i = 0; i < curr_thread->datasize; i++) {
        curr_thread->data[i] = new_data[i];
    }

    // clear signal handler
    for (int i = 0; i <= SIGNAL_MAX; i++) {
        curr_thread->signal_handler[i] = signal_default_handler;
    }

    tpf->elr_el1 = USER_KERNEL_BASE;
    tpf->sp_el0 = USER_STACK_BASE;
    tpf->x0 = 0;
    return 0;
}

int fork(trapframe_t *tpf)
{
    lock();
    thread_t *newt = thread_create(curr_thread->data, curr_thread->datasize);

    // copy signal handler
    for (int i = 0; i <= SIGNAL_MAX; i++) {
        newt->signal_handler[i] = curr_thread->signal_handler[i];
    }

    // remap
    mappages(newt->context.pgd, USER_KERNEL_BASE, newt->datasize, (size_t)VIRT_TO_PHYS(newt->data));
    mappages(newt->context.pgd, USER_STACK_BASE - USTACK_SIZE, USTACK_SIZE, (size_t)VIRT_TO_PHYS(newt->stack_alloced_ptr));
    mappages(newt->context.pgd, PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, PERIPHERAL_START);

    int parent_pid = curr_thread->pid;

    // copy data into new process
    for (int i = 0; i < newt->datasize; i++) {
        newt->data[i] = curr_thread->data[i];
    }

    // copy user stack into new process
    for (int i = 0; i < USTACK_SIZE; i++) {
        newt->stack_alloced_ptr[i] = curr_thread->stack_alloced_ptr[i];
    }

    // copy stack into new process
    for (int i = 0; i < KSTACK_SIZE; i++) {
        newt->kernel_stack_alloced_ptr[i] = curr_thread->kernel_stack_alloced_ptr[i];
    }

    store_context(get_current());

    // for child
    if (parent_pid != curr_thread->pid) {
        goto child;
    }

    void *temp_pgd = newt->context.pgd;
    newt->context = curr_thread->context;
    newt->context.pgd = VIRT_TO_PHYS(temp_pgd);
    newt->context.fp += newt->kernel_stack_alloced_ptr - curr_thread->kernel_stack_alloced_ptr; // move fp
    newt->context.sp += newt->kernel_stack_alloced_ptr - curr_thread->kernel_stack_alloced_ptr; // move kernel sp

    unlock();

    tpf->x0 = newt->pid;
    return newt->pid;

child:
    tpf->x0 = 0;
    return 0;
}

void exit(trapframe_t *tpf, int status) { thread_exit(); }

int syscall_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int *mbox_user)
{
    lock();

    unsigned int size_of_mbox = mbox_user[0];
    memcpy((char *)pt, mbox_user, size_of_mbox);
    mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt));
    memcpy(mbox_user, (char *)pt, size_of_mbox);

    tpf->x0 = 8;
    unlock();
    return 0;
}

void kill(trapframe_t *tpf, int pid)
{
    lock();
    if (pid >= PIDMAX || pid < 0 || !threads[pid].isused) {
        unlock();
        return;
    }
    threads[pid].iszombie = 1;
    unlock();
    schedule();
}

void signal_register(int signal, void (*handler)())
{
    if (signal > SIGNAL_MAX || signal < 0)
        return;

    curr_thread->signal_handler[signal] = handler;
}

void signal_kill(int pid, int signal)
{
    if (pid > PIDMAX || pid < 0 || !threads[pid].isused)
        return;

    lock();
    threads[pid].sigcount[signal]++;
    unlock();
}

void sigreturn(trapframe_t *tpf)
{
    unsigned long signal_ustack = tpf->sp_el0 % USTACK_SIZE == 0 ? tpf->sp_el0 - USTACK_SIZE : tpf->sp_el0 & (~(USTACK_SIZE - 1));
    kfree((char *)signal_ustack);
    load_context(&curr_thread->signal_saved_context);
}

char *get_file_start(char *thefilepath)
{
    char *filepath;
    char *filedata;
    unsigned int filesize;
    struct cpio_newc_header *header_pointer = CPIO_DEFAULT_START;

    while (header_pointer != 0) {
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        if (error)
            break;
        if (strcmp(thefilepath, filepath) == 0)
            return filedata;
        if (header_pointer == 0)
            uart_puts("execfile: %s: No such file or directory\r\n", thefilepath);
    }
    return 0;
}

unsigned int get_file_size(char *thefilepath)
{
    char *filepath;
    char *filedata;
    unsigned int filesize;
    struct cpio_newc_header *header_pointer = CPIO_DEFAULT_START;

    while (header_pointer != 0) {
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        if (error)
            break;
        if (strcmp(thefilepath, filepath) == 0)
            return filesize;
        if (header_pointer == 0)
            uart_puts("execfile: %s: No such file or directory\r\n", thefilepath);
    }
    return 0;
}

int open(trapframe_t *tpf, const char *pathname, int flags)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, pathname);
    get_abs_path(abs_path, curr_thread->cur_working_dir);
    uart_printf("[SYSCALL OPEN absolute path]: '%s' + '%s' = '%s'\n", curr_thread->cur_working_dir, pathname, abs_path);

    for (int i = 0; i < MAX_FD; i++) {
        if (!curr_thread->FDT[i]) {
            if (vfs_open(abs_path, flags, &curr_thread->FDT[i]) != 0)
                break; // open failed

            tpf->x0 = i;
            return i;
        }
    }

    tpf->x0 = -1;
    return -1;
}

int close(trapframe_t *tpf, int fd)
{
    if (curr_thread->FDT[fd]) {
        vfs_close(curr_thread->FDT[fd]);
        curr_thread->FDT[fd] = 0;
        tpf->x0 = 0;
        return 0;
    }

    tpf->x0 = -1;
    return -1; // fd not found
}

long write(trapframe_t *tpf, int fd, const void *buf, unsigned long count)
{
    if (curr_thread->FDT[fd]) {
        tpf->x0 = vfs_write(curr_thread->FDT[fd], buf, count);
        return tpf->x0;
    }

    tpf->x0 = -1;
    return -1; // fd not found
}

long read(trapframe_t *tpf, int fd, void *buf, unsigned long count)
{
    if (curr_thread->FDT[fd]) {
        tpf->x0 = vfs_read(curr_thread->FDT[fd], buf, count);
        return tpf->x0;
    }

    tpf->x0 = -1;
    return -1; // fd not found
}

int mkdir(trapframe_t *tpf, const char *pathname, unsigned mode)
{
    // get absolute path
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, pathname);
    get_abs_path(abs_path, curr_thread->cur_working_dir);

    tpf->x0 = vfs_mkdir(abs_path);
    return tpf->x0;
}

int mount(trapframe_t *tpf, const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, target);
    get_abs_path(abs_path, curr_thread->cur_working_dir);

    tpf->x0 = vfs_mount(abs_path, filesystem);
    return tpf->x0;
}

int chdir(trapframe_t *tpf, const char *path)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, path);
    get_abs_path(abs_path, curr_thread->cur_working_dir);
    strcpy(curr_thread->cur_working_dir, abs_path);

    // tpf->x0 = 0;
    return 0;
}

long lseek64(trapframe_t *tpf, int fd, long offset, int whence)
{
    if (whence == SEEK_SET) {
        curr_thread->FDT[fd]->f_pos = offset;
        tpf->x0 = offset;
    }
    else {
        tpf->x0 = -1;
    }

    return tpf->x0;
}

extern unsigned int height;
extern unsigned int isrgb;
extern unsigned int pitch;
extern unsigned int width;
int ioctl(trapframe_t *tpf, int fb, unsigned long request, void *info)
{
    if (request == 0) {
        struct framebuffer_info *fb_info = info;
        fb_info->height = height;
        fb_info->isrgb = isrgb;
        fb_info->pitch = pitch;
        fb_info->width = width;
    }

    tpf->x0 = 0;
    return tpf->x0;
}