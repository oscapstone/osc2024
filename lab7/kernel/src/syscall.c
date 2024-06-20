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
#include "dev_framebuffer.h"

int getpid(trapframe_t* tpf)
{
    tpf->x0 = curr_thread->pid;
    return curr_thread->pid;
}

size_t uartread(trapframe_t *tpf,char buf[], size_t size)
{
    int i = 0;
    for (int i = 0; i < size;i++)
    {
        buf[i] = uart_async_getc();
    }
    tpf->x0 = i;
    return i;
}

size_t uartwrite(trapframe_t *tpf,const char buf[], size_t size)
{
    int i = 0;
    for (int i = 0; i < size; i++)
    {
        uart_async_putc(buf[i]);
    }
    tpf->x0 = i;
    return i;
}

//In this lab, you won’t have to deal with argument passing
int exec(trapframe_t *tpf,const char *name, char *const argv[])
{
    mmu_del_vma(curr_thread);
    INIT_LIST_HEAD(&curr_thread->vma_list);

    // use virtual file system
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, name);
    get_absolute_path(abs_path, curr_thread->curr_working_dir);

    struct vnode *target_file;
    vfs_lookup(abs_path,&target_file);
    curr_thread->datasize = target_file->f_ops->getsize(target_file);

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

    //memcpy(curr_thread->data, new_data, curr_thread->datasize);
    struct file *f;
    vfs_open(abs_path, 0, &f);
    vfs_read(f, curr_thread->data, curr_thread->datasize);
    vfs_close(f);

    for (int i = 0; i <= SIGNAL_MAX; i++)
    {
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
    thread_t *newt = thread_create(curr_thread->data,curr_thread->datasize);

    //copy signal handler
    for (int i = 0; i <= SIGNAL_MAX;i++)
    {
        newt->signal_handler[i] = curr_thread->signal_handler[i];
    }

    for (int i = 0; i <= MAX_FD; i++)
    {
        if (curr_thread->file_descriptors_table[i])
        {
            newt->file_descriptors_table[i] = kmalloc(sizeof(struct file));
            *newt->file_descriptors_table[i] = *curr_thread->file_descriptors_table[i];
        }
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

void exit(trapframe_t *tpf, int status)
{
    thread_exit();
}

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
        tpf->x0 = (unsigned long) mmap(tpf, (void *)(the_area_ptr->virt_addr + the_area_ptr->area_size), len, prot, flags, fd, file_offset);
        return (void *)tpf->x0;
    }
    // create new valid region, map and set the page attributes (prot)
    mmu_add_vma(curr_thread, (unsigned long)addr, len, VIRT_TO_PHYS((unsigned long)kmalloc(len)), prot, 1);
    tpf->x0 = (unsigned long)addr;
    return (void*)tpf->x0;
}


// Basic 3
int open(trapframe_t *tpf, const char *pathname, int flags)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, pathname);
    // update abs_path
    // 複製路徑並更新為絕對路徑
    get_absolute_path(abs_path, curr_thread->curr_working_dir);
    for (int i = 0; i < MAX_FD; i++)
    {
        // find a usable fd
        // current thread的file descriptors table中尋找一個空位。
        if(!curr_thread->file_descriptors_table[i]){
            // 用vfs_open打開file並儲存file descriptors table
            if(vfs_open(abs_path, flags, &curr_thread->file_descriptors_table[i])!=0){
                break;
            }
            // 成功，將file descriptors table return給tpf->x0
            tpf->x0 = i;
            return i;
        }
    }
    //else則return -1。
    tpf->x0 = -1;
    return -1;
}

int close(trapframe_t *tpf, int fd)
{
    // find an opened fd
    if(curr_thread->file_descriptors_table[fd]){
        // 用 vfs_close關閉文件，並將file descriptors table設為空
        vfs_close(curr_thread->file_descriptors_table[fd]);
        curr_thread->file_descriptors_table[fd] = 0;
        tpf->x0 = 0;
        return 0;
    }

    tpf->x0 = -1;
    return -1;
}

long write(trapframe_t *tpf, int fd, const void *buf, unsigned long count)
{
    // find an opened fd
    if (curr_thread->file_descriptors_table[fd]){
        // 用 vfs_write寫入data，return寫入的bytes
        tpf->x0 = vfs_write(curr_thread->file_descriptors_table[fd], buf, count);
        return tpf->x0;
    }

    tpf->x0 = -1;
    return tpf->x0;
}

long read(trapframe_t *tpf, int fd, void *buf, unsigned long count)
{
    // find an opened fd
    if (curr_thread->file_descriptors_table[fd]){
        // 用 vfs_read讀data，return讀取的bytes
        tpf->x0 = vfs_read(curr_thread->file_descriptors_table[fd], buf, count);
        return tpf->x0;
    }

    tpf->x0 = -1;
    return tpf->x0;
}

int mkdir(trapframe_t *tpf, const char *pathname, unsigned mode)
{
    char abs_path[MAX_PATH_NAME];
    // 複製路徑並更新為絕對路徑
    strcpy(abs_path, pathname);
    get_absolute_path(abs_path, curr_thread->curr_working_dir);
    // 用vfs_mkdir create directory
    tpf->x0 = vfs_mkdir(abs_path);
    return tpf->x0;
}

int mount(trapframe_t *tpf, const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data)
{
    char abs_path[MAX_PATH_NAME];
    // 複製路徑並更新為絕對路徑
    strcpy(abs_path, target);
    get_absolute_path(abs_path, curr_thread->curr_working_dir);
    // 用vfs_mount mount file system
    tpf->x0 = vfs_mount(abs_path,filesystem);
    return tpf->x0;
}

int chdir(trapframe_t *tpf, const char *path)
{
    char abs_path[MAX_PATH_NAME];
    // 複製路徑並更新為絕對路徑
    strcpy(abs_path, path);
    get_absolute_path(abs_path, curr_thread->curr_working_dir);
    // 將更新後的路徑設置為current的工作目錄
    strcpy(curr_thread->curr_working_dir, abs_path);

    return 0;
}
// ---------------------------Basic 3

// Advanced 2
// 設置file descriptors tablef對應的file pos
// write framebuffer again without reopen
long lseek64(trapframe_t *tpf, int fd, long offset, int whence){
    // 如果 whence是 SEEK_SET，則將file pos設置為 offset
    // 並更新tpf的返回值為 offset
    if(whence == SEEK_SET) {
        // used for dev_framebuffer
        curr_thread->file_descriptors_table[fd]->f_pos = offset;
        tpf->x0 = offset;
    }
    else {
        // other is not supported
        tpf->x0 = -1;
    }

    return tpf->x0;
}

// 進行device特定的operation，用於獲取framebuffer的message
// query framebuffer info
extern unsigned int height;
extern unsigned int isrgb;
extern unsigned int pitch;
extern unsigned int width;
int ioctl(trapframe_t *tpf, int fb, unsigned long request, void *info)
{
    // 將 framebuffer的message(height, isrgb, pitch, width)寫入到 info
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
// ---------------------------Advanced 2

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

