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
extern int uart_recv_echo_flag; // to prevent the syscall.img from using uart_send() in uart_recv()

int syscall_num = 0;
SYSCALL_TABLE_T syscall_table [] = {
    { .func=getpid                  },
    { .func=uartread                },
    { .func=uartwrite               },
    { .func=exec                    },
    { .func=fork                    },
    { .func=exit                    },
    { .func=syscall_mbox_call       },
    { .func=kill                    },
    { .func=signal_register         },
    { .func=signal_kill             },
    { .func=mmap                    },
    { .func=signal_reture           }
};

void init_syscall()
{
    syscall_num = sizeof(syscall_table) / sizeof(SYSCALL_TABLE_T);
    // uart_sendline("syscall_table[syscall_no].func: 0x%x\r\n", syscall_table[0].func);

}
SYSCALL_DEFINE(getpid)
{
    tpf->x0 = curr_thread->pid;
    return curr_thread->pid;
}

SYSCALL_DEFINE(uartread)
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

SYSCALL_DEFINE(uartwrite)
{
    // uart_sendline("uartwrite: %s\r\n", (char *)tpf->x0);
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
SYSCALL_DEFINE(exec)
{
    char *name = (char *)tpf->x0;
    // char *argv[] = (char **)tpf->x1;

    mmu_del_vma(curr_thread);
    INIT_LIST_HEAD(&curr_thread->vma_list);

    curr_thread->datasize = get_file_size((char *)name);
    char *new_data = get_file_start((char *)name);
    curr_thread->data = kmalloc(curr_thread->datasize);
    curr_thread->stack_alloced_ptr = kmalloc(USTACK_SIZE);

    asm("dsb ish\n\t");      // ensure write has completed
    mmu_free_page_tables(curr_thread->context.pgd, 0);
    memset(PHYS_TO_VIRT(curr_thread->context.pgd), 0, 0x1000);
    asm("tlbi vmalle1is\n\t" // invalidate all TLB entries
        "dsb ish\n\t"        // ensure completion of TLB invalidatation
        "isb\n\t");          // clear pipeline

    mmu_add_vma(curr_thread,              USER_KERNEL_BASE,             curr_thread->datasize, (size_t)VIRT_TO_PHYS(curr_thread->data)             , 0b111, 1);
    mmu_add_vma(curr_thread, USER_STACK_BASE - USTACK_SIZE,                       USTACK_SIZE, (size_t)VIRT_TO_PHYS(curr_thread->stack_alloced_ptr), 0b111, 1);
    mmu_add_vma(curr_thread,              PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START,                                     PERIPHERAL_START, 0b011, 0);
    mmu_add_vma(curr_thread,        USER_SIGNAL_WRAPPER_VA,                            0x2000,         (size_t)VIRT_TO_PHYS(signal_handler_wrapper), 0b101, 0);

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

SYSCALL_DEFINE(fork)
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
        // ignore device and signal wrapper
        vma = (vm_area_struct_t *)pos;
        if (vma->virt_addr == USER_SIGNAL_WRAPPER_VA || vma->virt_addr == PERIPHERAL_START)
        {
            continue;
        }
        char *new_alloc = kmalloc(vma->area_size);
        mmu_add_vma(newt, vma->virt_addr, vma->area_size, (size_t)VIRT_TO_PHYS(new_alloc), vma->rwx, 1);
        memcpy(new_alloc, (void*)PHYS_TO_VIRT(vma->phys_addr), vma->area_size);
    }
    mmu_add_vma(newt,       PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START,                             PERIPHERAL_START, 0b011, 0);
    mmu_add_vma(newt, USER_SIGNAL_WRAPPER_VA,                            0x2000, (size_t)VIRT_TO_PHYS(signal_handler_wrapper), 0b101, 0);

    int parent_pid = curr_thread->pid;

    //copy stack into new process
    for (int i = 0; i < KSTACK_SIZE; i++)
    {
        newt->kernel_stack_alloced_ptr[i] = curr_thread->kernel_stack_alloced_ptr[i];
    }

    store_context(get_current());
    //for child
    if( parent_pid != curr_thread->pid)
    {
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

SYSCALL_DEFINE(exit)
{
    uart_recv_echo_flag = 1;
    thread_exit();
    return 0;
}

SYSCALL_DEFINE(syscall_mbox_call)
{
    // unsigned char ch = (unsigned char)tpf->x0;
    unsigned int *mbox_user = (unsigned int *)tpf->x1;
    lock();

    unsigned int size_of_mbox = mbox_user[0];
    memcpy((char *)pt, mbox_user, size_of_mbox);
    mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt));
    memcpy(mbox_user, (char *)pt, size_of_mbox);

    tpf->x0 = 8;
    unlock();
    return 0;
}

SYSCALL_DEFINE(kill)
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

SYSCALL_DEFINE(signal_register)
{
    int signal = tpf->x0;
    void (*handler)() = (void (*)())tpf->x1;

    if (signal > SIGNAL_MAX || signal < 0)return -1;

    curr_thread->signal_handler[signal] = handler;

    return 0;
}

SYSCALL_DEFINE(signal_kill)
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
SYSCALL_DEFINE(mmap)
{
    void *addr                 = (void *)tpf->x0;
    size_t len                 = (size_t) tpf->x1;
    int prot                   = (int)tpf->x2;
    // int flags                  = (int)tpf->x3;
    // int fd                     = (int)tpf->x4;
    // int file_offset            = (int)tpf->x5;
    // Ignore flags as we have demand pages

    // Req #3 Page size round up
    len = len % 0x1000 ? len + (0x1000 - len % 0x1000) : len;
    addr = (unsigned long)addr % 0x1000 ? addr + (0x1000 - (unsigned long)addr % 0x1000) : addr;

    // Req #2 check if overlap
    list_head_t *pos;
    vm_area_struct_t *vma;
    vm_area_struct_t *the_area_ptr = 0;
    list_for_each(pos, &curr_thread->vma_list)
    {
        vma = (vm_area_struct_t *)pos;
        // Detect existing vma overlapped
        if ( ! (vma->virt_addr >= (unsigned long)(addr + len) || vma->virt_addr + vma->area_size <= (unsigned long)addr ) )
        {
            the_area_ptr = vma;
            break;
        }
    }
    // take as a hint to decide new region's start address
    if (the_area_ptr)
    {
        tpf->x1 = (unsigned long)(the_area_ptr->virt_addr + the_area_ptr->area_size);
        tpf->x0 = (unsigned long) mmap(tpf);

        return (unsigned long)tpf->x0;
    }
    // create new valid region, map and set the page attributes (prot)
    mmu_add_vma(curr_thread, (unsigned long)addr, len, VIRT_TO_PHYS((unsigned long)kmalloc(len)), prot, 1);
    tpf->x0 = (unsigned long)addr;
    return (unsigned long)tpf->x0;
}

SYSCALL_DEFINE(signal_reture)
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
