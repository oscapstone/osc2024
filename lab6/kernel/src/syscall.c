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

int getpid(trapframe_t* tpf)
{
    // save return value as current pid
    tpf->x0 = curr_thread->pid;
    return curr_thread->pid;
}

// 通過async UART讀取data到buffer
// 讀到tpf->x0
size_t uartread(trapframe_t *tpf,char buf[], size_t size)
{
    int i = 0;
    // reading buffer through async uart
    for (int i = 0; i < size;i++){
        buf[i] = uart_async_getc();
    }
    // return size
    tpf->x0 = i;
    return i;
}

// 通過async UART將buffer中的data寫入UART
// 存到tpf->x0
size_t uartwrite(trapframe_t *tpf,const char buf[], size_t size)
{
    int i = 0;
    // write through async uart
    for (int i = 0; i < size; i++){
        uart_async_putc(buf[i]);
    }
    // size
    tpf->x0 = i;
    return i;
}

//In this lab, you won’t have to deal with argument passing
// 根據file name載入new process，並替換當前thread的data，重置signal handler，設置return address和user stack pointer
int exec(trapframe_t *tpf,const char *name, char *const argv[])
{
    // del. current thread的所有VMA
    mmu_del_vma(curr_thread);
    INIT_LIST_HEAD(&curr_thread->vma_list);
    
    // get filesize according filename
    // cpio parse and return data & filesize
    // 根據filename得到filesize和content
    curr_thread->datasize = get_file_size((char *)name);
    char *new_data = get_file_start((char *)name);
    
    // 為new data和stack分配memory
    curr_thread->data = kmalloc(curr_thread->datasize);
    curr_thread->stack_alloced_ptr = kmalloc(USTACK_SIZE);

    // 確保寫入已完成
    asm("dsb ish\n\t");
    
    // free current thread的page table
    mmu_free_page_tables(curr_thread->context.pgd, 0);
    memset(PHYS_TO_VIRT(curr_thread->context.pgd), 0, 0x1000);
    
    // 使所有TLB entries invalidate並clear pipeline
    asm("tlbi vmalle1is\n\t" // invalidate all TLB entries
        "dsb ish\n\t"        // ensure completion of TLB invalidatation
        "isb\n\t");          // clear pipeline

    // 為current thread增加新的VMA
    mmu_add_vma(curr_thread, USER_KERNEL_BASE, curr_thread->datasize, (size_t)VIRT_TO_PHYS(curr_thread->data), 0b111, 1);
    mmu_add_vma(curr_thread, USER_STACK_BASE - USTACK_SIZE, USTACK_SIZE, (size_t)VIRT_TO_PHYS(curr_thread->stack_alloced_ptr), 0b111, 1);
    mmu_add_vma(curr_thread, PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, PERIPHERAL_START, 0b011, 0);
    mmu_add_vma(curr_thread, USER_SIGNAL_WRAPPER_VA, 0x2000, (size_t)VIRT_TO_PHYS(signal_handler_wrapper), 0b101, 0);

    // copy new data到current thread的data
    memcpy(curr_thread->data, new_data, curr_thread->datasize);
    
    // clear signal handler
    for (int i = 0; i <= SIGNAL_MAX; i++){
        curr_thread->signal_handler[i] = signal_default_handler;
    }

    // set program (exception return address)
    tpf->elr_el1 = USER_KERNEL_BASE;
    
    // set user stack pointer (el0)
    tpf->sp_el0 = USER_STACK_BASE;
    
    // set return value
    tpf->x0 = 0;
    return 0;
}

// 創建當前thread的copy，並設置新thread的context，包括user stack pointer和kernel stack的copy
// 對於child process，return 0，對於parent process，return child process的PID
int fork(trapframe_t *tpf)
{
    lock();
    // fork process accrording current program
    // set lr to curr data
    thread_t *newt = thread_create(curr_thread->data,curr_thread->datasize);

    //copy signal handler
    for (int i = 0; i <= SIGNAL_MAX;i++)
    {
        newt->signal_handler[i] = curr_thread->signal_handler[i];
    }

    // copy VMA
    list_head_t *pos;
    vm_area_struct_t *vma;
    list_for_each(pos, &curr_thread->vma_list){
        // ignore device and signal wrapper
        vma = (vm_area_struct_t *)pos;
        if (vma->virt_addr == USER_SIGNAL_WRAPPER_VA || vma->virt_addr == PERIPHERAL_START) continue;
        
        // 分配新的meomry並copy VMA
        char *new_alloc = kmalloc(vma->area_size);
        mmu_add_vma(newt, vma->virt_addr, vma->area_size, (size_t)VIRT_TO_PHYS(new_alloc), vma->rwx, 1);
        memcpy(new_alloc, (void*)PHYS_TO_VIRT(vma->phys_addr), vma->area_size);
    }
    
    // 增加peripheral和signal wrapper的VMA
    mmu_add_vma(newt, PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, PERIPHERAL_START, 0b011, 0);
    mmu_add_vma(newt, USER_SIGNAL_WRAPPER_VA, 0x2000, (size_t)VIRT_TO_PHYS(signal_handler_wrapper), 0b101, 0);

    // store parent pid
    int parent_pid = curr_thread->pid;

    // copy stack到new process
    for (int i = 0; i < KSTACK_SIZE; i++){
        newt->kernel_stack_alloced_ptr[i] = curr_thread->kernel_stack_alloced_ptr[i];
    }
    
    // set child process return address
    // 儲存 current context 到  current thread
    store_context(get_current());
    
    // for child process
    if(parent_pid != curr_thread->pid) 
    	goto child;
    
    // set new thread的context
    void *temp_pgd = newt->context.pgd;
    newt->context = curr_thread->context;
    newt->context.pgd = VIRT_TO_PHYS(temp_pgd);
    newt->context.fp += newt->kernel_stack_alloced_ptr - curr_thread->kernel_stack_alloced_ptr; // move fp
    newt->context.sp += newt->kernel_stack_alloced_ptr - curr_thread->kernel_stack_alloced_ptr; // move kernel sp

    unlock();
    
    // return child process pid
    tpf->x0 = newt->pid;
    return newt->pid;

child:
    // child return 0
    tpf->x0 = 0;
    return 0;
}

// 中止當前process, 從schedule移除
void exit(trapframe_t *tpf, int status)
{
    thread_exit();
}

// 實現與mailbox的通訊，用於CPU與GPU之間的通訊。它等待mailbox可寫，發送消息，並等待響應。響應成功則return 1，否則return 0。
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

void kill(trapframe_t *tpf,int pid)
{
    lock();
    if (pid >= PIDMAX || pid < 0  || !threads[pid].isused)
    {
        unlock();
        return;
    }
    threads[pid].iszombie = 1;
    unlock();
    schedule();
}

void signal_register(int signal, void (*handler)())
{
    if (signal > SIGNAL_MAX || signal < 0)return;

    curr_thread->signal_handler[signal] = handler;
}

void signal_kill(int pid, int signal)
{
    if (pid > PIDMAX || pid < 0 || !threads[pid].isused)return;

    lock();
    threads[pid].sigcount[signal]++;
    unlock();
}

//only need to implement the anonymous page mapping in this Lab.
void *mmap(trapframe_t *tpf, void *addr, size_t len, int prot, int flags, int fd, int file_offset)
{
    // Ignore flags as we have demand pages

    // Req #3 Page size round up
    // 將大小和地址對齊到頁大小（4KB）
    len = len % 0x1000 ? len + (0x1000 - len % 0x1000) : len;
    addr = (unsigned long)addr % 0x1000 ? addr + (0x1000 - (unsigned long)addr % 0x1000) : addr;

    // Req #2 check if overlap
    // 檢查是否voerlap
    list_head_t *pos;
    vm_area_struct_t *vma;
    vm_area_struct_t *the_area_ptr = 0;
    list_for_each(pos, &curr_thread->vma_list)
    {
        vma = (vm_area_struct_t *)pos;
        // 檢測現有的 VMA 是否重疊
        if ( ! (vma->virt_addr >= (unsigned long)(addr + len) || vma->virt_addr + vma->area_size <= (unsigned long)addr ) )
        {
            the_area_ptr = vma;
            break;
        }
    }
    
    // take as a hint to decide new region's start address
    // 如果overlap，則提示決定new region的起始地址
    if (the_area_ptr)
    {
        tpf->x0 = (unsigned long) mmap(tpf, (void *)(the_area_ptr->virt_addr + the_area_ptr->area_size), len, prot, flags, fd, file_offset);
        return (void *)tpf->x0;
    }
    
    // 創建新的valid region，map並set page attributes（prot）
    mmu_add_vma(curr_thread, (unsigned long)addr, len, VIRT_TO_PHYS((unsigned long)kmalloc(len)), prot, 1);
    tpf->x0 = (unsigned long)addr;
    // return new region的address
    return (void*)tpf->x0;
}

void sigreturn(trapframe_t *tpf)
{
    //unsigned long signal_ustack = tpf->sp_el0 % USTACK_SIZE == 0 ? tpf->sp_el0 - USTACK_SIZE : tpf->sp_el0 & (~(USTACK_SIZE - 1));
    //kfree((char*)signal_ustack);
    load_context(&curr_thread->signal_saved_context);
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
