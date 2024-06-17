#include "bcm2837/rpi_mbox.h"
#include "syscall.h"
#include "sched.h"
#include "uart1.h"
#include "stdio.h"
#include "exception.h"
#include "memory.h"
#include "mbox.h"
#include "signal.h"
#include "string.h"

#include "debug.h"
#include "cpio.h"
#include "dtb.h"
#include "mmu.h"

extern void *CPIO_DEFAULT_START;
extern thread_t *curr_thread;
extern thread_t threads[PIDMAX + 1];

// trap is like a shared buffer for user space and kernel space
// Because general-purpose registers are used for both arguments and return value,
// We may receive the arguments we need, and overwrite them with return value.

int getpid(trapframe_t *tpf)
{
    // uart_sendlinek("this is getpid");
    tpf->x0 = curr_thread->pid;
    return curr_thread->pid;
}

size_t uartread(trapframe_t *tpf, char buf[], size_t size)
{
    int i = 0;
    stdio_op(stdin, buf, size);
    tpf->x0 = i;
    return i;
}

size_t uartwrite(trapframe_t *tpf, const char buf[], size_t size)
{
    int i = 0;
    char *cptr = buf;
    stdio_op(stdout, buf, size);
    tpf->x0 = i;
    return i;
}

// In this lab, you won’t have to deal with argument passing
int exec(trapframe_t *tpf, const char *name, char *const argv[])
{
    lock();
    mmu_del_vma(curr_thread);
    INIT_LIST_HEAD(&curr_thread->vma_list);

    // use virtual file system
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, name);
    get_absolute_path(abs_path, curr_thread->curr_working_dir);

    // uart_sendlinek("file name : %s\n", name);
    // uart_sendlinek("curr_working_dir : %s\n", curr_thread->curr_working_dir);
    // uart_sendlinek("abs_path : %s\n", abs_path);

    struct vnode *target_file;
    if (vfs_lookup(abs_path, &target_file) != 0)
    {
        WARING("File : %s Does not Exit!!", abs_path);
        return 0;
    };
    curr_thread->datasize = target_file->f_ops->getsize(target_file);
    uart_sendlinek("datasize : %d\n", curr_thread->datasize);

    // curr_thread->data = kmalloc(curr_thread->datasize > PAGESIZE ? curr_thread->datasize : PAGESIZE);
    curr_thread->data = kmalloc(curr_thread->datasize);
    curr_thread->stack_alloced_ptr = kmalloc(USTACK_SIZE);

    // data copy
    memcpy(curr_thread->signal_handler, curr_thread->signal_handler, SIGNAL_MAX * 8);
    struct file *f;
    vfs_open(abs_path, 0, &f);
    vfs_read(f, curr_thread->data, curr_thread->datasize);
    vfs_close(f);

    // clean vma & page_tables
    asm("dsb ish\n\t"); // ensure write has completed
    mmu_free_page_tables(curr_thread->context.pgd, 0);
    memset((void *)PHYS_TO_KERNEL_VIRT(curr_thread->context.pgd), 0, 0x1000);
    asm("tlbi vmalle1is\n\t" // invalidate all TLB entries
        "dsb ish\n\t"        // ensure completion of TLB invalidatation
        "isb\n\t");          // clear pipeline
    mmu_del_vma(curr_thread);

    // new vma
    mmu_add_vma(curr_thread, USER_DATA_BASE, curr_thread->datasize, (size_t)KERNEL_VIRT_TO_PHYS(curr_thread->data), 0b111, 1, USER_DATA);
    mmu_add_vma(curr_thread, USER_STACK_BASE - USTACK_SIZE, USTACK_SIZE, (size_t)KERNEL_VIRT_TO_PHYS(curr_thread->stack_alloced_ptr), 0b011, 1, USER_STACK);
    mmu_add_vma(curr_thread, PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, PERIPHERAL_START, 0b011, 0, PERIPHERAL);
    mmu_add_vma(curr_thread, USER_SIGNAL_WRAPPER_VA, 0x1000, ALIGN_DOWN((size_t)KERNEL_VIRT_TO_PHYS(signal_handler_wrapper), 0x1000), 0b101, 0, USER_SIGNAL_WRAPPER);
    mmu_add_vma(curr_thread, USER_EXEC_WRAPPER_VA, 0x2000, ALIGN_DOWN((size_t)KERNEL_VIRT_TO_PHYS(exec_wrapper), PAGESIZE), 0b101, 0, USER_EXEC_WRAPPER);

    tpf->elr_el1 = USER_DATA_BASE;
    tpf->sp_el0 = USER_STACK_BASE - STACK_BASE_OFFSET;
    tpf->x0 = 0;
    unlock();
    return 0;
}

// extern unsigned long long int lock_counter;
int fork(trapframe_t *tpf)
{
    lock();
    thread_t *newt = thread_create(curr_thread->data);
    // mmu_set_PTE_readonly(curr_thread->context.pgd,0);
    // mmu_pagetable_copy(newt->context.pgd,curr_thread->context.pgd,0);
    // uart_sendlinek("fork\n");
    memcpy(newt->signal_handler, curr_thread->signal_handler, SIGNAL_MAX * 8);
    memcpy(newt->stack_alloced_ptr, curr_thread->kernel_stack_alloced_ptr, USTACK_SIZE);
    memcpy(newt->kernel_stack_alloced_ptr, curr_thread->kernel_stack_alloced_ptr, KSTACK_SIZE);
    memcpy(newt->curr_working_dir, curr_thread->curr_working_dir,MAX_PATH_NAME+1);

    newt->datasize = curr_thread->datasize;
    newt->data = kmalloc(newt->datasize);
    memcpy(newt->data,curr_thread->data,newt->datasize);
    //newt->curr_working_dir = curr_thread->curr_working_dir;
    // memcpy(newt->file_descriptors_table, curr_thread->file_descriptors_table, MAX_FD * 8); // <------------------------------------------------
    mmu_add_vma(newt, USER_DATA_BASE, newt->datasize, (size_t)KERNEL_VIRT_TO_PHYS(newt->data), 0b111, 1, USER_DATA);
    mmu_add_vma(newt, USER_STACK_BASE - USTACK_SIZE, USTACK_SIZE, (size_t)KERNEL_VIRT_TO_PHYS(newt->stack_alloced_ptr), 0b011, 1, USER_STACK);
    mmu_add_vma(newt, PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, PERIPHERAL_START, 0b011, 0, PERIPHERAL);
    mmu_add_vma(newt, USER_SIGNAL_WRAPPER_VA, 0x1000, ALIGN_DOWN((size_t)KERNEL_VIRT_TO_PHYS(signal_handler_wrapper), 0x1000), 0b101, 0, USER_SIGNAL_WRAPPER);
    mmu_add_vma(newt, USER_EXEC_WRAPPER_VA, 0x2000, ALIGN_DOWN((size_t)KERNEL_VIRT_TO_PHYS(exec_wrapper), PAGESIZE), 0b101, 0, USER_EXEC_WRAPPER);

    int parent_pid = curr_thread->pid;

    store_context(get_current());
    // for child
    if (parent_pid != curr_thread->pid)
    {
        goto child;
    }

    // 除了PGD以外的context都複製。
    void *temp_pgd = newt->context.pgd;
    newt->context = curr_thread->context;
    newt->context.pgd = temp_pgd;
    //memcpy(newt->context.pgd,curr_thread->context.pgd,PAGESIZE);

    newt->context.fp += newt->kernel_stack_alloced_ptr - curr_thread->kernel_stack_alloced_ptr; // move fp
    newt->context.sp += newt->kernel_stack_alloced_ptr - curr_thread->kernel_stack_alloced_ptr; // move kernel sp

    unlock();

    tpf->x0 = newt->pid;
    return newt->pid;

child:
    tpf->x0 = 0;
    return 0;
}

void exit(trapframe_t *tpf, int status)
{
    thread_exit();
    while (1)
        schedule();
}

int syscall_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int *mbox)
{
    lock();

    unsigned int size_of_mbox = mbox[0];
    memcpy((char *)pt, mbox, size_of_mbox);
    mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt));
    memcpy(mbox, (char *)pt, size_of_mbox);

    // tpf->x0 = 8;
    unlock();
    return 0;
}

// only need to implement the anonymous page mapping in this Lab.
void *mmap(trapframe_t *tpf, void *addr, size_t len, int prot, int flags, int fd, int file_offset)
{
    len = ALIGN_UP(len, PAGESIZE);
    unsigned long base_user_va = ALIGN_DOWN((unsigned long)addr, PAGESIZE);
    uart_sendlinek("+\n");
    uart_sendlinek("| User request new vma base vitural address: 0x%x\n", (unsigned long)addr);
    uart_sendlinek("| Aligned to PAGESIZE: 0x%x\n", base_user_va);
    uart_sendlinek("| User request new vma size: 0x%x\n", len);
    uart_sendlinek("| Exec, Write, Read : 0x%d\n", prot);
    uart_sendlinek("+\n");

    // Req #2 check if overlap
    vm_area_struct_t *the_area_ptr = check_vma_overlap(curr_thread, base_user_va, (unsigned long)len);
    // take as a hint to decide new region's start address
    if (the_area_ptr)
    {
        WARING("Vitural Memory Area Overlap\n");
        WARING("Find another vma base vitural address\n");
        tpf->x0 = (unsigned long)mmap(tpf, (void *)(the_area_ptr->virt_addr + the_area_ptr->area_size), len, prot, flags, fd, file_offset);
        return (void *)tpf->x0;
    }
    // create new valid region, map and set the page attributes (prot)
    mmu_add_vma(curr_thread, base_user_va, len, KERNEL_VIRT_TO_PHYS((unsigned long)kmalloc(len)), prot, 1, UNKNOW_AREA);
    tpf->x0 = base_user_va;
    return (void *)tpf->x0;
}

void kill(trapframe_t *tpf, int pid)
{
    if (pid < 0 || pid >= PIDMAX || !threads[pid].isused)
        return;

    lock();

    if (pid == curr_thread->pid)
    {
        uart_sendlinek("[!] you kill youself !! \n");
        thread_exit();
        unlock();
        while (1)
            schedule();
    }
    else
    {
        threads[pid].iszombie = 1;
        unlock();
    }
    schedule();
}

void signal_register(int signal, void (*handler)())
{
    if (signal > SIGNAL_MAX || signal < 0)
        return;
    // uart_sendlinek("handler : 0x%x\n", handler);
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
    // unsigned long signal_ustack = tpf->sp_el0 % USTACK_SIZE == 0 ? tpf->sp_el0 - USTACK_SIZE : tpf->sp_el0 & (~(USTACK_SIZE - 1));
    // kfree((char *)signal_ustack);
    load_context(&curr_thread->signal_savedContext);
}

void syscall_unlock(trapframe_t *tpf)
{
    unlock();
}

void syscall_lock(trapframe_t *tpf)
{
    lock();
}

char *get_file_start(char *thefilepath)
{
    int FLAG_getfile = 0;
    char *filepath;
    char *filedata;
    unsigned int filesize;
    // struct cpio_newc_header *header_pointer = CPIO_DEFAULT_START;

    CPIO_for_each(&filepath, &filesize, &filedata)
    {
        if (strcmp(thefilepath, filepath) == 0)
        {
            FLAG_getfile = 1;
            return filedata;
        }
    }

    if (!FLAG_getfile)
    {
        uart_sendlinek("execfile: %s: No such file or directory\r\n", thefilepath);
    }

    // while (header_pointer != 0)
    // {
    //     int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
    //     //if parse header error
    //     if (error)
    //     {
    //         uart_sendlinek("error");
    //         break;
    //     }

    //     if (strcmp(thefilepath, filepath) == 0)
    //     {
    //         return filedata;
    //     }

    //     //if this is TRAILER!!! (last of file)
    //     if (header_pointer == 0)
    //         uart_sendlinek("execfile: %s: No such file or directory\r\n", thefilepath);
    // }

    return 0;
}

unsigned int get_file_size(char *thefilepath)
{
    int FLAG_getfile = 0;
    char *filepath;
    char *filedata;
    unsigned int filesize;

    CPIO_for_each(&filepath, &filesize, &filedata)
    {
        if (strcmp(thefilepath, filepath) == 0)
        {
            FLAG_getfile = 1;
            return filesize;
        }
    }

    if (!FLAG_getfile)
    {
        uart_sendlinek("execfile: %s: No such file or directory\r\n", thefilepath);
    }

    // while (header_pointer != 0)
    // {
    //     int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
    //     // if parse header error
    //     if (error)
    //     {
    //         uart_sendlinek("error");
    //         break;
    //     }

    //     if (strcmp(thefilepath, filepath) == 0)
    //     {
    //         return filesize;
    //     }

    //     // if this is TRAILER!!! (last of file)
    //     if (header_pointer == 0)
    //         uart_sendlinek("execfile: %s: No such file or directory\r\n", thefilepath);
    // }
    return 0;
}
