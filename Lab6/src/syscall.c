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

//  Get current process’s id.
int getpid(trapframe_t *tp)
{
    tp->x0 = curr_thread->pid;
    return curr_thread->pid;
}

//  Return the number of bytes read by reading size byte into the user-supplied buffer buf.
size_t uartread(trapframe_t *tp, char buf[], size_t size)
{
    int i = 0;
    for (int i = 0; i < size; i++)
    {
        buf[i] = uart_async_getc();
    }
    tp->x0 = i;
    return i;
}

//  Return the number of bytes written after writing size byte from the user-supplied buffer buf.
size_t uartwrite(trapframe_t *tp, const char buf[], size_t size)
{
    int i = 0;
    for (int i = 0; i < size; i++)
    {
        uart_async_putc(buf[i]);
    }
    tp->x0 = i;
    return i;
}

//  Run the program with parameters.
//  In this lab, you won’t have to deal with argument passing, but you can still use it.
int exec(trapframe_t *tp, const char *name, char *const argv[])
{
    mmu_del_vma(curr_thread);
    INIT_LIST_HEAD(&curr_thread->vma_list);
    curr_thread->datasize = get_file_size((char *)name);
    char *new_data = get_file_start((char *)name);
    curr_thread->data = kmalloc(curr_thread->datasize);
    curr_thread->stack_allocted_ptr = kmalloc(USTACK_SIZE);

    asm("dsb ish\n\t"); // ensure write has completed
    mmu_free_page_tables(curr_thread->context.pgd, 0);
    memset(PHYS_TO_VIRT(curr_thread->context.pgd), 0, 0x1000);
    asm("tlbi vmalle1is\n\t" // invalidate all TLB entries
        "dsb ish\n\t"        // ensure completion of TLB invalidatation
        "isb\n\t");          // clear pipeline

    mmu_add_vma(curr_thread, USER_KERNEL_BASE, curr_thread->datasize, (size_t)VIRT_TO_PHYS(curr_thread->data), 0b111, 1);
    mmu_add_vma(curr_thread, USER_STACK_BASE - USTACK_SIZE, USTACK_SIZE, (size_t)VIRT_TO_PHYS(curr_thread->stack_allocted_ptr), 0b111, 1);
    mmu_add_vma(curr_thread, PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, PERIPHERAL_START, 0b011, 0);
    mmu_add_vma(curr_thread, USER_SIGNAL_WRAPPER_VA, 0x2000, (size_t)VIRT_TO_PHYS(signal_handler_wrapper), 0b101, 0);

    memcpy(curr_thread->data, new_data, curr_thread->datasize);
    // initial signal handler
    for (int i = 0; i <= SIGNAL_MAX; i++)
    {
        curr_thread->signal_handler[i] = signal_default_handler;
    }

    // tp->elr_el1 = (unsigned long)curr_thread->data;                           // 要執行的program
    // tp->sp_el0 = (unsigned long)curr_thread->stack_allocted_ptr + USTACK_SIZE; // sp of the program
    tp->elr_el1 = USER_KERNEL_BASE;
    tp->sp_el0 = USER_STACK_BASE;

    tp->x0 = 0;
    return 0;
}

//  The standard method of duplicating the current process in UNIX-like operating systems is to use fork().
//  Following the call to fork(), two processes run the same code.
//  Set the parent process’s return value to the child’s id and the child process’s return value to 0 to distinguish them.
int fork(trapframe_t *tp)
{
    lock();
    uart_sendline("size: 0x%x\r\n", curr_thread->datasize);
    thread_t *newt = thread_create(curr_thread->data, curr_thread->datasize);

    // copy signal handler
    for (int i = 0; i <= SIGNAL_MAX; i++)
    {
        newt->signal_handler[i] = curr_thread->signal_handler[i];
    }

    list_head_t *pos;
    vm_area_struct_t *vma;
    list_for_each(pos, &curr_thread->vma_list)
    {
        // ignore device and signal wrapper
        vma = (vm_area_struct_t *)pos;
        if (vma->virt_addr == USER_SIGNAL_WRAPPER_VA || vma->virt_addr == PERIPHERAL_START)
        {
            continue;
        }
        char *new_alloc = kmalloc(vma->area_size);
        mmu_add_vma(newt, vma->virt_addr, vma->area_size, (size_t)VIRT_TO_PHYS(new_alloc), vma->rwx, 1);
        memcpy(new_alloc, (void *)PHYS_TO_VIRT(vma->phys_addr), vma->area_size);
    }
    mmu_add_vma(newt, PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, PERIPHERAL_START, 0b011, 0);
    mmu_add_vma(newt, USER_SIGNAL_WRAPPER_VA, 0x2000, (size_t)VIRT_TO_PHYS(signal_handler_wrapper), 0b101, 0);

    int parent_pid = curr_thread->pid;

    // copy stack into new process
    for (int i = 0; i < KSTACK_SIZE; i++)
    {
        newt->kernel_stack_allocted_ptr[i] = curr_thread->kernel_stack_allocted_ptr[i];
    }

    store_context(get_current());
    // for child
    if (parent_pid != curr_thread->pid)
    {
        tp->x0 = 0;
        return 0;
    }

    void *temp_pgd = newt->context.pgd;
    newt->context = curr_thread->context;
    newt->context.pgd = VIRT_TO_PHYS(temp_pgd);
    newt->context.fp += newt->kernel_stack_allocted_ptr - curr_thread->kernel_stack_allocted_ptr; // move fp
    newt->context.sp += newt->kernel_stack_allocted_ptr - curr_thread->kernel_stack_allocted_ptr; // move kernel sp

    unlock();

    tp->x0 = newt->pid;
    return newt->pid;
}

//  Terminate the current process.
void exit(trapframe_t *tp, int status)
{
    thread_exit();
}

//  Get the hardware’s information by mailbox
int syscall_mbox_call(trapframe_t *tp, unsigned char ch, unsigned int *mbox_user)
{
    lock();

    unsigned int size_of_mbox = mbox_user[0];
    memcpy((char *)m, mbox_user, size_of_mbox);
    mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&m));
    memcpy(mbox_user, (char *)m, size_of_mbox);

    tp->x0 = 8;
    unlock();
    return 0;
}

//  Other processes identified by pid should be terminated.
//  You don’t need to implement this system call if you prefer to kill a process using the POSIX Signal stated in Advanced Exercise 1.
void kill(trapframe_t *tp, int pid)
{
    lock();
    if (pid >= PIDMAX || pid < 0 || !threads[pid].isused)
    {
        unlock();
        return;
    }
    threads[pid].iszombie = 1;
    unlock();
    schedule();
}

// system call number: 8
// register signal handler
void register_signal(int signal, void (*handler)())
{
    if (signal > SIGNAL_MAX || signal < 0)
        return;
    curr_thread->signal_handler[signal] = handler;
}

// system call number: 9
// kill by signal
void signal_kill(int pid, int signal)
{
    if (pid > PIDMAX || pid < 0 || !threads[pid].isused)
        return;
    lock();
    threads[pid].sigcount[signal]++;
    unlock();
}

// The kernel uses addr and len to create a new valid region for the current process.
// only need to implement the anonymous page mapping in this Lab.
void *mmap(trapframe_t *tp, void *addr, size_t len, int prot, int flags, int fd, int file_offset)
{
    uart_sendline("mmap(addr: 0x%x, len: %d, prot: %d, flags: %d, fd: %d, file_offset: %d)\n", (unsigned long)addr, (len), prot, flags, fd, file_offset);
    len = len % 0x1000 ? len + (0x1000 - len % 0x1000) : len;                                    // rounds up
    addr = (unsigned long)addr % 0x1000 ? addr + (0x1000 - (unsigned long)addr % 0x1000) : addr; // align

    // check if overlap
    list_head_t *pos;
    vm_area_struct_t *vma;
    vm_area_struct_t *the_area_ptr = 0;
    list_for_each(pos, &curr_thread->vma_list)
    {
        vma = (vm_area_struct_t *)pos;
        //uart_sendline("vma->virt_addr: 0x%x, area: 0x%x\r\n", vma->virt_addr, vma->area_size);
        // if overlap
        if ((vma->virt_addr < (unsigned long)(addr + len)) && (vma->virt_addr + vma->area_size > (unsigned long)addr))
        {
            the_area_ptr = vma;
            break;
        }
    }
    // test the end of the area as addr
    // If the new region overlaps with existing regions, or addr is not page-aligned, the kernel takes addr as a hint and decides the new region’s start address.
    // find a space after addr + len till find it
    if (the_area_ptr)
    {
        uart_sendline("overlap\r\n");
        tp->x0 = (unsigned long)mmap(tp, (void *)(the_area_ptr->virt_addr + the_area_ptr->area_size), len, prot, flags, fd, file_offset);
        return (void *)tp->x0;
    }
    // create new valid region, map and set the page attributes (prot)
    mmu_add_vma(curr_thread, (unsigned long)addr, len, VIRT_TO_PHYS((unsigned long)kmalloc(len)), prot, 1);
    tp->x0 = (unsigned long)addr;
    return (void *)tp->x0;
}

void sigreturn(trapframe_t *tp)
{
    // unsigned long signal_ustack = tpf->sp_el0 % USTACK_SIZE == 0 ? tpf->sp_el0 - USTACK_SIZE : tpf->sp_el0 & (~(USTACK_SIZE - 1));
    // kfree((char*)signal_ustack);
    load_context(&curr_thread->signal_saved_context);
}

char *get_file_start(char *thefilepath)
{
    char *filepath;
    char *filedata;
    unsigned int filesize;
    struct cpio_newc_header *header_pointer = CPIO_START;

    while (header_pointer != 0)
    {
        // if parse header error
        if (cpio_newc_parse(header_pointer, &filepath, &filesize, &filedata, &header_pointer))
        {
            uart_puts("error");
            break;
        }

        if (strcmp(thefilepath, filepath) == 0)
        {
            return filedata;
        }

        // if this is TRAILER!!! (last of file)
        if (header_pointer == 0)
            uart_puts("execfile: %s: No such file or directory\r\n", thefilepath);
    }
    return 0;
}

// get file size
unsigned int get_file_size(char *thefilepath)
{
    char *filepath;
    char *filedata;
    unsigned int filesize;
    struct cpio_newc_header *header_pointer = CPIO_START;

    while (header_pointer != 0)
    {
        // if parse header error
        if (cpio_newc_parse(header_pointer, &filepath, &filesize, &filedata, &header_pointer))
        {
            uart_puts("error");
            break;
        }

        if (strcmp(thefilepath, filepath) == 0)
        {
            return filesize;
        }

        // if this is TRAILER!!! (last of file)
        if (header_pointer == 0)
            uart_puts("execfile: %s: No such file or directory\r\n", thefilepath);
    }
    return 0;
}
