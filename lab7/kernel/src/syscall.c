#include "syscall.h"
#include "cpio.h"
#include "sched.h"
#include "stddef.h"
#include "uart1.h"
#include "exception.h"
#include "memory.h"
#include "mbox.h"
#include "signal.h"
#include "mmu.h"
#include "string.h"
#include "colourful.h"
#include "vfs.h"
#include "dev_framebuffer.h"


extern int uart_recv_echo_flag; // to prevent the syscall.img from using uart_send() in uart_recv()

char* syscall_table [SYSCALL_TABLE_SIZE];

void init_syscall()
{
    memset(syscall_table, 0, sizeof(syscall_table));
    
    syscall_table[0] = (char *)sys_getpid;
    syscall_table[1] = (char *)sys_uartread;
    syscall_table[2] = (char *)sys_uartwrite;
    syscall_table[3] = (char *)sys_exec;
    syscall_table[4] = (char *)sys_fork;
    syscall_table[5] = (char *)sys_exit;
    syscall_table[6] = (char *)sys_mbox_call;
    syscall_table[7] = (char *)sys_kill;
    syscall_table[8] = (char *)sys_signal_register;
    syscall_table[9] = (char *)sys_signal_kill;
    syscall_table[10] = (char *)sys_mmap;

    // lab 7
    syscall_table[11] = (char *)sys_open;         
    syscall_table[12] = (char *)sys_close;         
    syscall_table[13] = (char *)sys_write;         
    syscall_table[14] = (char *)sys_read;         
    syscall_table[15] = (char *)sys_mkdir;         
    syscall_table[16] = (char *)sys_mount;         
    syscall_table[17] = (char *)sys_chdir;         
    syscall_table[18] = (char *)sys_lseek64;
    syscall_table[19] = (char *)sys_ioctl;



    syscall_table[50] = (char *)sys_signal_reture;
}


SYSCALL_DEFINE1(getpid, trapframe_t*, tpf)
{
    tpf->x0 = curr_thread->pid;
    return curr_thread->pid;
}

// It's for scanf()
SYSCALL_DEFINE1(uartread, trapframe_t*, tpf)
{
    char *buf = (char *)tpf->x0;
    size_t size = tpf->x1;

    int i = 0;
    for (int i = 0; i < size;i++)
    {
        buf[i] = uart_async_getc();
    }
    tpf->x0 = i;
    return i;
}

// It's for printf()
SYSCALL_DEFINE1(uartwrite, trapframe_t*, tpf)
{
    const char *buf = (const char *)tpf->x0;
    size_t size = tpf->x1;

    int i = 0;
    for (int i = 0; i < size; i++)
    {
        uart_async_putc(buf[i]);
    }
    tpf->x0 = i;
    return i;
}

//In this lab, you won’t have to deal with argument passing
SYSCALL_DEFINE1(exec, trapframe_t*, tpf)
{
    char *name = (char *)tpf->x0;
    // char *argv[] = (char **)tpf->x1;

    mmu_del_vma(curr_thread);
    INIT_LIST_HEAD(&curr_thread->vma_list);

    curr_thread->datasize =             get_file_size((char *)name);
    char *new_data =                    get_file_start((char *)name);
    curr_thread->data =                 kmalloc(curr_thread->datasize);
    curr_thread->stack_alloced_ptr =    kmalloc(USTACK_SIZE);

    asm("dsb ish\n\t");      // ensure write has completed
    mmu_free_page_tables(curr_thread->context.pgd, 0);
    memset(PHYS_TO_VIRT(curr_thread->context.pgd), 0, 0x1000);
    asm("tlbi vmalle1is\n\t" // invalidate all TLB entries
        "dsb ish\n\t"        // ensure completion of TLB invalidatation
        "isb\n\t");          // clear pipeline

    // add vma                        User Virtual Address,                              Size,                             Physical Address,            xwr,  name                                  is_alloced
    mmu_add_vma(curr_thread,              USER_KERNEL_BASE,             curr_thread->datasize,                (size_t)VIRT_TO_PHYS(curr_thread->data), 0b111, "Code & Data Segment\0",              1);
    mmu_add_vma(curr_thread, USER_STACK_BASE - USTACK_SIZE,                       USTACK_SIZE,   (size_t)VIRT_TO_PHYS(curr_thread->stack_alloced_ptr), 0b111, "Stack Segment\0",                    1);
    mmu_add_vma(curr_thread,              PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START,                                       PERIPHERAL_START, 0b011, "Peripheral\0",                       0);
    mmu_add_vma(curr_thread,        USER_SIGNAL_WRAPPER_VA,                            0x2000,           (size_t)VIRT_TO_PHYS(signal_handler_wrapper), 0b101, "Signal Handler Wrapper\0",           0);

    memcpy(curr_thread->data, new_data, curr_thread->datasize);
    for (int i = 0; i <= SIGNAL_MAX; i++)
    {
        curr_thread->signal_handler[i] = signal_default_handler;
    }


    tpf->elr_el1 = USER_KERNEL_BASE;
    tpf->sp_el0 = USER_STACK_BASE;
    tpf->x0 = 0;
    return 0;
}

void get_ttbr1_el1()
{
    unsigned long long ttbr1_el1;
    asm volatile("mrs %0, ttbr1_el1\n\t"
                 : "=r"(ttbr1_el1));

    uart_sendline("ttbr1_el1: 0x%x\r\n", ttbr1_el1);
}
SYSCALL_DEFINE1(fork, trapframe_t*, tpf)
{
    lock();
    thread_t *newt = thread_create(curr_thread->data,curr_thread->datasize, NORMAL_PRIORITY);

    //copy signal handler
    for (int i = 0; i <= SIGNAL_MAX;i++)
    {
        newt->signal_handler[i] = curr_thread->signal_handler[i];
    }

    list_head_t *pos;
    vm_area_struct_t *vma;
    list_for_each(pos, &curr_thread->vma_list){
        vma = (vm_area_struct_t *)pos;
        mmu_add_vma(newt, vma->virt_addr, vma->area_size, vma->phys_addr, vma->rwx, vma->name, 1);
    }


    int parent_pid = curr_thread->pid;

    //copy kernel stack into new process. It's not a user stack
    for (int i = 0; i < KSTACK_SIZE; i++)
    {
        newt->kernel_stack_alloced_ptr[i] = curr_thread->kernel_stack_alloced_ptr[i];
    }

    store_context(get_current());
    //for child
    if( parent_pid != curr_thread->pid)
    {
        lock();
        goto child;
    }

    void *temp_pgd = newt->context.pgd;
    newt->context = curr_thread->context;
    newt->context.pgd = VIRT_TO_PHYS(temp_pgd);
    newt->context.fp += newt->kernel_stack_alloced_ptr - curr_thread->kernel_stack_alloced_ptr; // move fp
    newt->context.sp += newt->kernel_stack_alloced_ptr - curr_thread->kernel_stack_alloced_ptr; // move kernel sp
    
    mmu_reset_page_tables_read_only(curr_thread->context.pgd, newt->context.pgd, 0);

    unlock();
    schedule();

    tpf->x0 = newt->pid;
    return newt->pid;

child:
    unlock();
    tpf->x0 = 0;
    return 0;
}

SYSCALL_DEFINE1(exit, trapframe_t*, tpf)
{
    uart_recv_echo_flag = 1;
    thread_exit();
    return 0;
}

SYSCALL_DEFINE1(mbox_call, trapframe_t*, tpf)
{
    unsigned char ch = (unsigned char)tpf->x0;
    unsigned int *mbox_user = (unsigned int *)tpf->x1;
    lock();

    unsigned int size_of_mbox = mbox_user[0];
    memcpy((char *)pt, mbox_user, size_of_mbox);
    mbox_call(ch, (unsigned int)((unsigned long)&pt));

    memcpy(mbox_user, (char *)pt, size_of_mbox);

    tpf->x0 = 8;
    unlock();
    return 0;
}

SYSCALL_DEFINE1(kill, trapframe_t*, tpf)
{
    int pid = tpf->x0;
    lock();
    if (pid >= PIDMAX || pid < 0  || !threads[pid].isused)
    {
        unlock();
        return -1;
    }
    threads[pid].iszombie = 1;
    unlock();
    schedule();
    return 0;
}

SYSCALL_DEFINE1(signal_register, trapframe_t*, tpf)
{
    int signal = tpf->x0;
    void (*handler)() = (void (*)())tpf->x1;

    if (signal > SIGNAL_MAX || signal < 0)return -1;

    curr_thread->signal_handler[signal] = handler;

    return 0;
}

SYSCALL_DEFINE1(signal_kill, trapframe_t*, tpf)
{
    int pid = tpf->x0;
    int signal = tpf->x1;

    if (pid > PIDMAX || pid < 0 || !threads[pid].isused)return -1;

    lock();
    threads[pid].sigcount[signal]++;
    unlock();
    return 0;
}

//only need to implement the anonymous page mapping in this Lab.
// void *mmap(trapframe_t *tpf, void *addr, size_t len, int prot, int flags, int fd, int file_offset)
SYSCALL_DEFINE1(mmap, trapframe_t*, tpf)
{
    void *addr                 = (void *)tpf->x0;
    size_t len                 = (size_t) tpf->x1;
    int prot                   = (int)tpf->x2;
    int flags                  = (int)tpf->x3;
    int fd                     = (int)tpf->x4;
    int file_offset            = (int)tpf->x5;
    // Ignore flags as we have demand pages
    uart_sendline(RED "Syscall" RESET " mmap");
    uart_sendline("(addr: 0x%8x, len: %d, prot: %d, flags: %d, fd: %d, file_offset: %d)\r\n", addr, len, prot, flags, fd, file_offset);

    // Req #3 Page size round up
    len = len % 0x1000 ? len + (0x1000 - len % 0x1000) : len;
    addr = (unsigned long)addr % 0x1000 ? addr + (0x1000 - (unsigned long)addr % 0x1000) : addr;

    // Req #2 check if overlap
    list_head_t *pos;
    vm_area_struct_t *vma;
    vm_area_struct_t *vma_area_ptr = 0;
    list_for_each(pos, &curr_thread->vma_list)
    {
        vma = (vm_area_struct_t *)pos;
        // Detect existing vma overlapped
        if ( ! (vma->virt_addr >= (unsigned long)(addr + len) || vma->virt_addr + vma->area_size <= (unsigned long)addr ) )
        {
            vma_area_ptr = vma;
            break;
        }
    }
    // take as a hint to decide new region's start address
    if (vma_area_ptr)
    {
        uart_sendline(YEL "vma overlap or the addr is NULL!! we mmap a new region's start address\r\n" RESET);
        tpf->x0 = (unsigned long)(vma_area_ptr->virt_addr + vma_area_ptr->area_size); // add to the end of the existing vma
        tpf->x0 = (unsigned long) sys_mmap(tpf);

        return (unsigned long)tpf->x0;
    }
    // create new valid region, map and set the page attributes (prot)
    mmu_add_vma(curr_thread, (unsigned long)addr, len, VIRT_TO_PHYS((unsigned long)kmalloc(len)), prot, "Other Segment\0", 1);
    tpf->x0 = (unsigned long)addr;
    return (unsigned long)tpf->x0;
}


SYSCALL_DEFINE1(open, trapframe_t *, tpf)
{
    const char *pathname = (char*)tpf->x0;
    int flags = tpf->x1;


    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, pathname);
    // update abs_path
    get_absolute_path(abs_path, curr_thread->curr_working_dir);
    for (int i = 0; i < MAX_FD; i++)
    {
        // find a usable fd
        if(!curr_thread->file_descriptors_table[i])
        {
            if(vfs_open(abs_path, flags, &curr_thread->file_descriptors_table[i])!=0)
            {
                break;
            }

            tpf->x0 = i;
            return i;
        }
    }

    tpf->x0 = -1;
    return -1;
}

SYSCALL_DEFINE1(close, trapframe_t *, tpf)
{
    int fd = tpf->x0;
    // find an opened fd
    if(curr_thread->file_descriptors_table[fd])
    {
        vfs_close(curr_thread->file_descriptors_table[fd]);
        curr_thread->file_descriptors_table[fd] = 0;
        tpf->x0 = 0;
        return 0;
    }

    tpf->x0 = -1;
    return -1;
}

SYSCALL_DEFINE1(write, trapframe_t *, tpf)
{
    int fd = tpf->x0;
    const void *buf = (char *)tpf->x1;
    unsigned long count = tpf->x2;

    // find an opened fd
    if (curr_thread->file_descriptors_table[fd])
    {
        tpf->x0 = vfs_write(curr_thread->file_descriptors_table[fd], buf, count);
        return tpf->x0;
    }

    tpf->x0 = -1;
    return tpf->x0;
}

SYSCALL_DEFINE1(read, trapframe_t *, tpf)
{
    int fd = tpf->x0;
    void *buf = (char *)tpf->x1;
    unsigned long count = tpf->x2;

    // find an opened fd
    if (curr_thread->file_descriptors_table[fd])
    {
        tpf->x0 = vfs_read(curr_thread->file_descriptors_table[fd], buf, count);
        return tpf->x0;
    }

    tpf->x0 = -1;
    return tpf->x0;
}

SYSCALL_DEFINE1(mkdir, trapframe_t *,tpf)
{
    const char *pathname = (char *)tpf->x0;
    // unsigned mode = tpf->x1;

    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, pathname);
    get_absolute_path(abs_path, curr_thread->curr_working_dir);
    tpf->x0 = vfs_mkdir(abs_path);
    return tpf->x0;
}

SYSCALL_DEFINE1(mount, trapframe_t *, tpf)
{
    // const char *src = (char *)tpf->x0;
    const char *target = (char *)tpf->x1;
    const char *filesystem = (char *)tpf->x2;
    // unsigned long flags = tpf->x3;
    // const void *data = (char *)tpf->x4;


    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, target);
    get_absolute_path(abs_path, curr_thread->curr_working_dir);

    tpf->x0 = vfs_mount(abs_path,filesystem);
    return tpf->x0;
}

SYSCALL_DEFINE1(chdir, trapframe_t *, tpf)
{
    const char *path = (char *)tpf->x0;
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, path);
    get_absolute_path(abs_path, curr_thread->curr_working_dir);
    strcpy(curr_thread->curr_working_dir, abs_path);

    return 0;
}

SYSCALL_DEFINE1(lseek64, trapframe_t *, tpf)
{
    int fd = tpf->x0;
    long offset = tpf->x1;
    int whence = tpf->x2;

    if(whence == SEEK_SET) // used for dev_framebuffer
    {
        curr_thread->file_descriptors_table[fd]->f_pos = offset;
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
SYSCALL_DEFINE1(ioctl, trapframe_t *, tpf)
{
    // int fb = tpf->x0; 
    unsigned long request = tpf->x1;
    void *info = (void *)tpf->x2;

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




SYSCALL_DEFINE1(signal_reture, trapframe_t*, tpf)
{
    //unsigned long signal_ustack = tpf->sp_el0 % USTACK_SIZE == 0 ? tpf->sp_el0 - USTACK_SIZE : tpf->sp_el0 & (~(USTACK_SIZE - 1));
    //kfree((char*)signal_ustack);
    load_context(&curr_thread->signal_saved_context);

    return 0;
}




char* get_file_start(char *thefilepath)
{
    char *filepath;
    char *filedata;
    unsigned int filesize;
    struct cpio_newc_header *header_pointer = CPIO_DEFAULT_START;

    while (header_pointer != 0)
    {
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        if (error) break;
        if (strcmp(thefilepath, filepath) == 0) return filedata;
        if (header_pointer == 0) uart_puts("execfile: %s: No such file or directory\r\n", thefilepath);
    }
    return 0;
}

unsigned int get_file_size(char *thefilepath)
{
    char *filepath;
    char *filedata;
    unsigned int filesize;
    struct cpio_newc_header *header_pointer = CPIO_DEFAULT_START;

    while (header_pointer != 0)
    {
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        if (error) break;
        if (strcmp(thefilepath, filepath) == 0) return filesize;
        if (header_pointer == 0) uart_puts("execfile: %s: No such file or directory\r\n", thefilepath);
    }
    return 0;
}
