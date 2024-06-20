#include "thread.h"

thread_t* thread_arr;
thread_t* cur_thread;

void init_thread_arr(){
    thread_arr = (thread_t*)malloc(MAX_TID * sizeof(thread_t));

    for (int i = 0; i < MAX_TID; i++){
        thread_arr[i].status = FREE;
        thread_arr[i].id = i;
        thread_arr[i].prev = 0;
        thread_arr[i].next = 0;
        // thread_arr[i].parent = 0;
        thread_arr[i].prog_size = 0;
    }
}   

thread_t* _thread_create(void (*func)(void)){
    
    for (int id = 0; id < MAX_TID; id++){
        if (thread_arr[id].status == FREE){
            thread_arr[id].status = READY;
            thread_arr[id].ctx.lr = func;

            thread_arr[id].kernel_sp = (uint8_t*)malloc(USTACK_SIZE);
            thread_arr[id].user_sp = (uint8_t*)malloc(KSTACK_SIZE);
            thread_arr[id].ctx.sp = (uint64_t)(thread_arr[id].kernel_sp + KSTACK_SIZE);
            thread_arr[id].ctx.fp = thread_arr[id].ctx.sp;

            for (int j = 0; j <= MAX_SIGNAL; j++){
                thread_arr[id].signal_handler[j] = 0;
                thread_arr[id].signal_count[j] = 0;
            }

            return (thread_t*)(&(thread_arr[id]));
        }
    }

    print_str("\nNo thread is available!");
    return 0;
}