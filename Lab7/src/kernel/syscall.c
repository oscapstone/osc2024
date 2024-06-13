#include "kernel/syscall.h"

trap_frame_t *current_tf;

int getpid(){
    // uart_puts("getpid: ");
    // uart_itoa(current_task->pid);
    // uart_puts("\n");
    current_tf->x0 = current_task->pid; 
    return current_task->pid;
}

unsigned int uart_read(char buf[], my_uint64_t size){
    int i;
    //unlock();
    //lock();
    // no overflow protection
    for(i = 0; i < size; i++){
        //buf[i] = uart_getc();
        buf[i] = (char)uart_irq_getc();
    }
    //unlock();
    current_tf->x0 = i;
    return i;
}

unsigned int uart_write(const char buf[], my_uint64_t size){
    int i;
    
    // no overflow protection
    for(i = 0; i < size; i++){
        uart_putc(buf[i]);
        //uart_irq_putc(buf[i]);
    }

    current_tf->x0 = i;
    return i;
}
// to execute a new program, we should use elr_el1 to store the address of the new program and use eret to jump to that address(in boot.S).
// So we use current_tf to modified the user space register
// then use a thread to execute it
int exec(const char* name, char *const argv[]){
    lock();
    char abs_path[MAX_PATHNAME + 1];
    struct vnode* target_file;
    struct file *f;
    unsigned int data_size;

    uart_puts("exec: ");
    uart_puts(name);
    uart_puts("\n");

    //memzero((my_uint64_t)&current_task->tf, sizeof(trap_frame_t));
    
    string_copy(abs_path, (char*)name);
    get_absolute_path(abs_path, current_task->curr_working_dir);
    vfs_lookup(abs_path, &target_file);
    data_size = target_file->f_ops->getsize(target_file);

    char *file_data = (char*)pool_alloc(data_size);
    vfs_open(abs_path, 0, &f);
    vfs_read(f, file_data, data_size);
    vfs_close(f);

    current_task->signal_is_checking = 0;
    for(int i = 0; i <= NR_SIGNALS; i++){
        current_task->signal_handler[i] = signal_default_handler;  // set all signal handler to default
        current_task->sigcount[i] = 0;        // set all signal count to 0
    }

    // for(int i = 0; i < MAX_FD; i++){
    //     if(current_task->file_descriptors_table[i] != 0){
    //         //vfs_close(current_task->file_descriptors_table[i]);
    //         current_task->file_descriptors_table[i] = 0;
    //     }
    // }

    // vfs_open("/dev/uart", 0, &current_task->file_descriptors_table[0]); // stdin
    // vfs_open("/dev/uart", 0, &current_task->file_descriptors_table[1]); // stdout
    // vfs_open("/dev/uart", 0, &current_task->file_descriptors_table[2]); // stderr
    my_uint64_t new_stk = (my_uint64_t)pool_alloc(THREAD_STK_SIZE);

    current_tf->elr_el1 = (my_uint64_t)file_data;
    current_tf->sp_el0 = (my_uint64_t)new_stk + (current_task->tf.sp_el0 - current_task->sp);
    current_tf->x0 = 0;
    unlock();
    return 0;
}
// In C, fork will return 0 to the child process and return the child's pid to the parent process
int fork(my_uint64_t stack){
    return copy_process(0, 0, 0, stack);
}
// mark the current thread as zombie and schedule another thread to run
// this will never return
void exit(){
    // lock();
    // cur_thread->status = -1; // indicate that this thread is zombie(dead, in this lab)
    // unlock();
    // schedule();
    exit_process();
}

int mbox_call(unsigned char ch, unsigned int *mbox){
    lock();
    
    unsigned int mbox_ptr = ((unsigned int)((unsigned long)mbox) & ~0xF) | (ch & 0xF);

    // Wait until the mailbox is not full
    while((mmio_read((long)MAILBOX_STATUS) & MAILBOX_FULL)){
        asm volatile("nop");
    }
    // write our address containing message to mailbox address
    mmio_write((long)MAILBOX_WRITE, mbox_ptr);
    // Wait for response
    while(1){
        // until the mailbox is not empty
        while(mmio_read((long)MAILBOX_STATUS) & MAILBOX_EMPTY){
            asm volatile("nop");
        }
        // if it's the response corresponded to our request
        if(mbox_ptr == mmio_read((long)MAILBOX_READ)){
            // if the response is successed
            current_tf->x0 = mailbox[1];
            unlock();
            return mbox[1] == REQUEST_SUCCEED;
        }
    }
    // failed to get from mailbox(should not reach here)
    current_tf->x0 = 0;
    unlock();
    return 0;
}

void kill(int pid){
    lock();
    if(pid < 0 || pid >= NR_TASKS){
        uart_puts("Invalid PID\n");
        unlock();
        return;
    }

    /*if(pid == current_task->pid){
        uart_puts("Cannot kill current task\n");
        unlock();
        return;
    }*/
    
    if(PCB[pid] != 0){
        PCB[pid]->status = TASK_ZOMBIE;
        unlock();
        process_schedule();
        return;
    }
    unlock();
    // PID not found
    uart_puts("PID not found\n");
}

void sigreg(int SIGNAL, void (*handler)()){
    if(SIGNAL < 0 || SIGNAL >= NR_SIGNALS){
        uart_puts("Invalid SIGNAL REG\n");
        return;
    }
    lock();
    // if(SIGNAL == 9)
    //     current_task->signal_handler[SIGNAL] = signal_default_handler;
    // else
    current_task->signal_handler[SIGNAL] = handler;
    unlock();
}

void sigkill(int pid, int SIGNAL){
    if(pid < 0 || pid >= NR_TASKS || PCB[pid] == 0 || PCB[pid]->status == TASK_ZOMBIE){
        uart_puts("Invalid PID\n");
        return;
    }
    lock();
    // send a signal to the process
    uart_itoa(pid);
    uart_putc(' ');
    uart_itoa(SIGNAL);
    PCB[pid]->sigcount[SIGNAL]++;
    unlock();
}

void sigret(void){
    uint64_t sig_stk;
    // if it's not using any stack space
    if(current_tf->sp_el0 % THREAD_STK_SIZE == 0)
        sig_stk = current_tf->sp_el0 - THREAD_STK_SIZE;
    // else make it aligned to the stack size
    else
        sig_stk = current_tf->sp_el0 & ~(THREAD_STK_SIZE - 1);

    pool_free((void*)sig_stk);
    // kill process if it's kill signal
    if(current_task->signal_handler[SIGNAL_KILL] != signal_default_handler)
        kill(current_task->pid);
    
    // restore the context of the process
    load_context(&current_task->signal_saved_context);
}

// syscall number : 11
int open(const char *pathname, int flags){
    char abs_path[MAX_PATHNAME + 1];

    string_copy(abs_path, (char*)pathname);
    get_absolute_path(abs_path, current_task->curr_working_dir);

    // uart_puts("open: ");
    // uart_puts(abs_path);
    // uart_puts("\n");

    for(int i = 0; i < MAX_FD; i++){
        // find a empty file descriptor
        if(current_task->file_descriptors_table[i] == 0){
            if(vfs_open(abs_path, flags, &current_task->file_descriptors_table[i]) != 0){
                // current_task->file_descriptors_table[i] = 0;
                // current_tf->x0 = -1;
                // return -1;
                break;
            }
            if(current_task->file_descriptors_table[i] == 0){
                uart_puts("Error during open\n");
                delay(1000000);
                current_tf->x0 = -1;
                return -1;
            }
            current_tf->x0 = i;
            return i;
        }
    }
    uart_puts("No file descriptor available\n");
    delay(1000000);
    delay(1000000);
    delay(1000000);
    current_tf->x0 = -1;
    return -1;
}

// syscall number : 12
int close(int fd){
    // uart_puts("close: ");
    // uart_itoa(fd);
    // uart_puts("\n");

    if(current_task->file_descriptors_table[fd] != 0){
        vfs_close(current_task->file_descriptors_table[fd]);
        current_task->file_descriptors_table[fd] = 0;
        current_tf->x0 = 0;
        return 0;
    }
    else{
        current_tf->x0 = -1;
        return -1;
    }
}

// syscall number : 13
// remember to return read size or error code
long write(int fd, const void *buf, unsigned long count){
    // uart_puts("write: ");
    // uart_puts(buf);
    // uart_puts(" to fd: ");
    // uart_itoa(fd);
    // uart_puts("\n");

    if(current_task->file_descriptors_table[fd] != 0){
        current_tf->x0 = vfs_write(current_task->file_descriptors_table[fd], buf, count);
        return current_tf->x0;
    }
    else{
        current_tf->x0 = -1;
        return -1;
    }
}

// syscall number : 14
// remember to return read size or error code
long read(int fd, void *buf, unsigned long count){
    // uart_puts("read: ");
    // uart_puts("from fd: ");
    // uart_itoa(fd);
    // uart_puts("\n");
    if(current_task->file_descriptors_table[fd] != 0){
        current_tf->x0 = vfs_read(current_task->file_descriptors_table[fd], buf, count);
        return current_tf->x0;
    }
    else{
        current_tf->x0 = -1;
        return -1;
    }
}

// syscall number : 15
// you can ignore mode, since there is no access control
int mkdir(const char *pathname, unsigned mode){
    char abs_path[MAX_PATHNAME + 1];

    string_copy(abs_path, (char*)pathname);
    get_absolute_path(abs_path, current_task->curr_working_dir);

    uart_puts("mkdir: ");
    uart_puts(abs_path);
    uart_puts("\n");

    if(vfs_mkdir(abs_path) != 0){
        current_tf->x0 = -1;
        return -1;
    }
    current_tf->x0 = 0;
    return 0;
}

// syscall number : 16
// you can ignore arguments other than target (where to mount) and filesystem (fs name)
int mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data){
    char abs_path[MAX_PATHNAME + 1];

    string_copy(abs_path, (char*)target);
    get_absolute_path(abs_path, current_task->curr_working_dir);

    uart_puts("mount: ");
    uart_puts(abs_path);
    uart_puts("\n");

    if(vfs_mount(abs_path, filesystem) != 0){
        current_tf->x0 = -1;
        return -1;
    }
    current_tf->x0 = 0;
    return 0;
}

// syscall number : 17
int chdir(const char *path){
    char abs_path[MAX_PATHNAME + 1];

    string_copy(abs_path, (char*)path);
    get_absolute_path(abs_path, current_task->curr_working_dir);
    string_copy(current_task->curr_working_dir, (char*)abs_path);

    uart_puts("chdir: ");
    uart_puts(abs_path);
    uart_puts("\n");

    current_tf->x0 = 0;
    return 0;
}

// syscall number : 18
// you only need to implement seek set
long lseek64(int fd, long offset, int whence){
    if(current_task->file_descriptors_table[fd] != 0){
        current_tf->x0 = vfs_lseek64(current_task->file_descriptors_table[fd], offset, whence);
        return current_tf->x0;
    }
    else{
        current_tf->x0 = -1;
        return -1;
    }
}

extern unsigned int width;
extern unsigned int height;
extern unsigned int pitch;
extern unsigned int isrgb;

// syscall number : 19
int ioctl(int fd, unsigned long request, void *info){

    // uart_puts("ioctl: ");
    // uart_puts("fd: ");
    // uart_itoa(fd);
    // uart_puts(" request: ");
    // uart_itoa(request);
    // uart_puts("\n");
    if(request == 0){
        ((struct framebuffer_info*)info)->width = width;
        ((struct framebuffer_info*)info)->height = height;
        ((struct framebuffer_info*)info)->pitch = pitch;
        ((struct framebuffer_info*)info)->isrgb = isrgb;
    }

    current_tf->x0 = 0;
    return 0;
}