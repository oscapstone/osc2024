void fork(trapframe *sp) {
    int pid;
    thread * t = allocate_page(sizeof(thread));
    for(int i=0; i< 64; i++){
        if(thread_pool[i] == 0){
            pid = i;
            uart_puts("Create thread with PID ");
            uart_int(i);
            newline();
            break;
        }
        if(i == 64 - 1){
            uart_puts("Error, no more threads\n\r");
        }
    }
    for(int i = 0; i< sizeof(thread); i++){
        ((char*)(&(t -> regs)))[i] = 0; 
    }
    t -> pid = pid;
    t -> parent = get_current() -> pid;
    t -> state = get_current() -> state;
    t -> priority = get_current() -> priority;
    // copy register
    // copy trapframe?

    sp -> x[0] = pid;
}