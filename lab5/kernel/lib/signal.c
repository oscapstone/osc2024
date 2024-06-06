#include "signal.h"

//檢查current thread的signal，並根據檢查結果執行對應的signal process
void check_signal(trapframe_t *tpf) {
    //確保對 curr_thread->signal_is_checking 的thread是安全的。
    //如果已經在檢查信號，則直接返回。
    lock();
    // 檢查是否正在nested檢查信號
    if (curr_thread->signal_is_checking) {
        unlock();
        return;
    }
    // 防止 nested running signal handler
    curr_thread->signal_is_checking = 1;
    unlock();

    for (int i = 0; i <= SIGNAL_MAX; ++i) {
        // 在running signal handler之前保存原始上下文
        store_context(&curr_thread->signal_saved_context);
        if (curr_thread->sigcount[i] > 0) {
            lock();
            curr_thread->sigcount[i]--;
            unlock();
            run_signal(tpf, i);
        }
    }
    lock();
    // signal檢查完成
    curr_thread->signal_is_checking = 0;
    unlock();
}

// run_signal設置並運行對應的signal handler
void run_signal(trapframe_t *tpf, int signal) {
    // 根據signal index設置目前的callback
    lock();
    curr_thread->curr_signal_handler = curr_thread->signal_handler[signal];
    unlock();
    // 在kernel run default handler
    if (curr_thread->curr_signal_handler == signal_default_handler) {
        //kill
        signal_default_handler();
        return;
    }
    // kernel malloc signal handler stack
    char *temp_signal_userstack = malloc(USTACK_SIZE);
    // set elr_el1 callback
    // eret return至user mode後, 執行signal_handler_wrapper
    __asm__ __volatile__(
        "msr elr_el1, %0\n\t"
        "msr sp_el0, %1\n\t" //設成新的user sp地址
        "msr spsr_el1, %2\n\t" //設成目前的process status
        "eret\n\t" ::"r"(signal_handler_wrapper),
        "r"(temp_signal_userstack + USTACK_SIZE),
        "r"(tpf->spsr_el1));
}

// 執行signal handler，並在完成後callback func. callback return原本的context
void signal_handler_wrapper() {
    // run callback
    curr_thread->curr_signal_handler();
    
    
    // system call sigreturn
    // pre set syscall number
    // 執行 signal return, 恢復原本的context
    asm("mov x8,50\n\t"
        "svc 0\n\t");
}

void signal_default_handler() {

    kill(0, curr_thread->pid);
}
