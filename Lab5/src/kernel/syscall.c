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
    char *file_addr = (char*)cpio_find((char*)name);
    // indicating that the file is either a directory or not exist
    if(file_addr == 0)
        return 0;

    cur_thread->data_size = cpio_get_size((char*)name);
    cur_thread->data = (char*)pool_alloc(cur_thread->data_size);
    
    for(int i = 0; i < cpio_get_size((char*)name); i++)
        cur_thread->data[i] = file_addr[i];
    
    current_tf->elr_el1 = (unsigned long)cur_thread->data;
    current_tf->sp_el0 = (unsigned long)cur_thread->context.sp;
    
    current_tf->x0 = 0;
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
    
    // restore the context of the process
    load_context(&current_task->signal_saved_context);
}