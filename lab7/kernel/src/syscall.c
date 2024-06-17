#include "bcm2837/rpi_mbox.h"
#include "syscall.h"
#include "sched.h"
#include "uart1.h"
#include "utils.h"
#include "exception.h"
#include "mbox.h"
#include "memory.h"
#include "cpio.h"
#include "vfs.h"
#include "dev_framebuffer.h"

extern thread_t *curr_thread;
// extern void *CPIO_DEFAULT_START;
extern thread_t threads[PIDMAX + 1];

// trap is like a shared buffer for user space and kernel space
// Because general-purpose registers are used for both arguments and return value,
// We may receive the arguments we need, and overwrite them with return value.

int getpid(trapframe_t *tpf)
{
    tpf->x0 = curr_thread->pid;
    return curr_thread->pid;
}

size_t uartread(trapframe_t *tpf, char buf[], size_t size)
{
    int i = 0;
    for (int i = 0; i < size; i++)
    {
        buf[i] = uart_async_getc();
    }
    tpf->x0 = i + 1;
    return i + 1;
}

size_t uartwrite(trapframe_t *tpf, const char buf[], size_t size)
{
    int i = 0;
    for (int i = 0; i < size; i++)
    {
        uart_async_putc(buf[i]);
    }
    tpf->x0 = i + 1;
    return i + 1;
}

// In this lab, you won’t have to deal with argument passing
int exec(trapframe_t *tpf, const char *name, char *const argv[])
{
    lock();
    // Lab5- use cpio to get file data
    // curr_thread->datasize = get_file_size((char *)name);
    // char *file_start = get_file_start((char *)name);

    // for (unsigned int i = 0; i < curr_thread->datasize; i++)
    // {
    //     curr_thread->data[i] = file_start[i];
    // }

    // Lab7- use virtual file system
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, name);
    get_absolute_path(abs_path, curr_thread->vfs.curr_working_dir);

    struct vnode *target_file;
    vfs_lookup(abs_path, &target_file);
    curr_thread->datasize = target_file->f_ops->getsize(target_file);

    struct file *f;
    vfs_open(abs_path, 0, &f);
    vfs_read(f, curr_thread->data, curr_thread->datasize);
    vfs_close(f);

    // inital signal handler
    for (int i = 0; i < SIGNAL_MAX; i++)
    {
        curr_thread->signal.handler_table[i] = signal_default_handler;
    }

    tpf->elr_el1 = (unsigned long)curr_thread->data;
    tpf->sp_el0 = (unsigned long)curr_thread->stack_allocated_base + USTACK_SIZE;
    unlock();
    tpf->x0 = 0;
    return 0;
}

int fork(trapframe_t *tpf)
{
    lock();
    thread_t *parent_thread = curr_thread;
    int parent_pid = parent_thread->pid;

    thread_t *child_thread = thread_create(parent_thread->data);

    // copy signal handler
    for (int i = 0; i < SIGNAL_MAX; i++)
    {
        child_thread->signal.handler_table[i] = parent_thread->signal.handler_table[i];
    }

    // copy user stack into new process
    for (int i = 0; i < USTACK_SIZE; i++)
    {
        child_thread->stack_allocated_base[i] = parent_thread->stack_allocated_base[i];
    }

    // copy stack into new process
    for (int i = 0; i < KSTACK_SIZE; i++)
    {
        child_thread->kernel_stack_allocated_base[i] = parent_thread->kernel_stack_allocated_base[i];
    }

    // copy datasize into new process
    child_thread->datasize = parent_thread->datasize;

    // copy vfs info
    child_thread->vfs = parent_thread->vfs;
    // for (int i = 0; i <= MAX_FD; i++)
    // {
    //     if (parent_thread->vfs.file_descriptors_table[i])
    //     {
    //         child_thread->vfs.file_descriptors_table[i] = kmalloc(sizeof(struct file));
    //         *child_thread->vfs.file_descriptors_table[i] = *parent_thread->vfs.file_descriptors_table[i];
    //     }
    // }
    
    store_context(get_current());

    // for child
    if (parent_pid != curr_thread->pid)
    {
        goto child;
    }
    // copy context into new process
    child_thread->context = parent_thread->context;
    // the offset of current syscall should also be updated to new cpu context
    child_thread->context.sp += child_thread->kernel_stack_allocated_base - parent_thread->kernel_stack_allocated_base;
    child_thread->context.fp = child_thread->context.sp;
    unlock();
    tpf->x0 = child_thread->pid;
    // schedule();
    return child_thread->pid; // pid = new

child:
    // the offset of current syscall should also be updated to new return point
    tpf = (trapframe_t *)((char *)tpf + (unsigned long)child_thread->kernel_stack_allocated_base - (unsigned long)parent_thread->kernel_stack_allocated_base); // move tpf
    tpf->sp_el0 += child_thread->kernel_stack_allocated_base - parent_thread->kernel_stack_allocated_base;
    tpf->x0 = 0;
    // schedule();
    return 0; // pid = 0
}

void exit(trapframe_t *tpf)
{
    thread_exit();
}

int syscall_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int *mbox)
{
    lock();
    // Add channel to mbox lower 4 bit
    unsigned long r = (((unsigned long)((unsigned long)mbox) & ~0xF) | (ch & 0xF));
    do
    {
        asm volatile("nop");
    } while (*MBOX_STATUS & BCM_ARM_VC_MS_FULL);
    // Write to Register
    *MBOX_WRITE = r;
    while (1)
    {
        do
        {
            asm volatile("nop");
        } while (*MBOX_STATUS & BCM_ARM_VC_MS_EMPTY);
        // Read from Register
        if (r == *MBOX_READ)
        {
            tpf->x0 = (mbox[1] == MBOX_REQUEST_SUCCEED);
            unlock();
            return mbox[1] == MBOX_REQUEST_SUCCEED;
        }
    }
    tpf->x0 = 0;
    unlock();
    return 0;
}

void kill(trapframe_t *tpf, int pid)
{
    if (pid < 0 || pid >= PIDMAX || !threads[pid].isused)
        return;
    lock();
    threads[pid].iszombie = 1;
    unlock();
    schedule();
}

void signal_register(int signal, void (*handler)())
{
    if (signal > SIGNAL_MAX || signal < 0)
        return;
    curr_thread->signal.handler_table[signal] = handler;
}

void signal_kill(int pid, int signal)
{
    if (pid > PIDMAX || pid < 0 || !threads[pid].isused)
        return;
    lock();
    threads[pid].signal.pending[signal]++;
    unlock();
}

void sigreturn()
{
    // unsigned long signal_ustack = tpf->sp_el0 % USTACK_SIZE == 0 ? tpf->sp_el0 - USTACK_SIZE : tpf->sp_el0 & (~(USTACK_SIZE - 1));
    kfree((char *)curr_thread->signal.stack_base);
    load_context(&curr_thread->signal.saved_context);
}

int open(trapframe_t *tpf, const char *pathname, int flags)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, pathname);
    // update abs_path
    get_absolute_path(abs_path, curr_thread->vfs.curr_working_dir);
    for (int fd = 0; fd < MAX_FD; fd++)
    {
        // find a usable fd
        if (!curr_thread->vfs.file_descriptors_table[fd])
        {
            if (vfs_open(abs_path, flags, &curr_thread->vfs.file_descriptors_table[fd]) != 0)
            {
                break;
            }

            tpf->x0 = fd;
            return fd;
        }
    }
    tpf->x0 = -1;
    return -1;
}

int close(trapframe_t *tpf, int fd)
{
    // find an opened fd
    if (curr_thread->vfs.file_descriptors_table[fd])
    {
        vfs_close(curr_thread->vfs.file_descriptors_table[fd]);
        curr_thread->vfs.file_descriptors_table[fd] = 0;
        tpf->x0 = 0;
        return 0;
    }

    tpf->x0 = -1;
    return -1;
}

long write(trapframe_t *tpf, int fd, const void *buf, unsigned long count)
{
    // find an opened fd
    if (curr_thread->vfs.file_descriptors_table[fd])
    {
        tpf->x0 = vfs_write(curr_thread->vfs.file_descriptors_table[fd], buf, count);
        return tpf->x0;
    }

    tpf->x0 = -1;
    return tpf->x0;
}

long read(trapframe_t *tpf, int fd, void *buf, unsigned long count)
{
    // find an opened fd
    if (curr_thread->vfs.file_descriptors_table[fd])
    {
        tpf->x0 = vfs_read(curr_thread->vfs.file_descriptors_table[fd], buf, count);
        return tpf->x0;
    }

    tpf->x0 = -1;
    return tpf->x0;
}

int mkdir(trapframe_t *tpf, const char *pathname, unsigned mode)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, pathname);
    get_absolute_path(abs_path, curr_thread->vfs.curr_working_dir);
    tpf->x0 = vfs_mkdir(abs_path);
    return tpf->x0;
}

int mount(trapframe_t *tpf, const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, target);
    get_absolute_path(abs_path, curr_thread->vfs.curr_working_dir);

    tpf->x0 = vfs_mount(abs_path, filesystem);
    return tpf->x0;
}

// change directory
int chdir(trapframe_t *tpf, const char *path)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, path);
    get_absolute_path(abs_path, curr_thread->vfs.curr_working_dir);
    strcpy(curr_thread->vfs.curr_working_dir, abs_path);

    return 0;
}

long lseek64(trapframe_t *tpf, int fd, long offset, int whence)
{
    if(whence == SEEK_SET) // used for dev_framebuffer
    {
        curr_thread->vfs.file_descriptors_table[fd]->f_pos = offset;
        tpf->x0 = offset;
    }
    else // other is not supported
    {
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
    if(request == 0) // used for get info (SPEC)
    {
        struct framebuffer_info *fb_info = info;
        fb_info->height = height;
        fb_info->isrgb = isrgb;
        fb_info->pitch = pitch;
        fb_info->width = width;
    }

    tpf->x0 = 0;
    return tpf->x0;
}

// Lab 5
// char *get_file_start(char *thefilepath)
// {
//     char *filepath;
//     unsigned int filesize;
//     char *filedata;
//     struct cpio_newc_header *header_pointer = CPIO_DEFAULT_START;

//     while (header_pointer != 0)
//     {
//         int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
//         // if parse header error
//         if (error)
//         {
//             uart_puts("error");
//             break;
//         }

//         if (strcmp(thefilepath, filepath) == 0)
//         {
//             return filedata;
//         }

//         // if this is TRAILER!!! (last of file)
//         if (header_pointer == 0)
//             uart_puts("execfile: %s: No such file or directory\r\n", thefilepath);
//     }
//     return 0;
// }

// unsigned int get_file_size(char *thefilepath)
// {
//     char *filepath;
//     char *filedata;
//     unsigned int filesize;
//     struct cpio_newc_header *header_pointer = CPIO_DEFAULT_START;

//     while (header_pointer != 0)
//     {
//         int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
//         // if parse header error
//         if (error)
//         {
//             uart_puts("error");
//             break;
//         }

//         if (strcmp(thefilepath, filepath) == 0)
//         {
//             return filesize;
//         }

//         // if this is TRAILER!!! (last of file)
//         if (header_pointer == 0)
//             uart_puts("execfile: %s: No such file or directory\r\n", thefilepath);
//     }
//     return 0;
// }
