unsigned long get_current_time(){//and set next
    unsigned long cntfrq, cntpct;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    //asm volatile ("msr cntp_tval_el0, %0" : : "r" (cntfrq));//*2
    asm volatile ("mrs %0, cntpct_el0" : "=r" (cntpct));
    cntpct /= cntfrq;
    return cntpct;
}

void set_timer_interrupt(unsigned long second){//and set next
    unsigned long ctl = 1;
    asm volatile ("msr cntp_ctl_el0, %0" : : "r" (ctl));
    unsigned long cntfrq, cntpct;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    asm volatile ("msr cntp_tval_el0, %0" : : "r" (cntfrq * second));
    asm volatile ("mov x0, 2");
    asm volatile ("ldr x1, =0x40000040");
    asm volatile ("str w0, [x1]");
}


void disable_core_timer() {
    // 禁用定时器
    unsigned long ctl = 0;
    asm volatile ("msr cntp_ctl_el0, %0" : : "r" (ctl));

    // 屏蔽定时器中断
    asm volatile ("mov x0, 0"); // 假设0是用来屏蔽中断
    asm volatile ("ldr x1, =0x40000040");
    asm volatile ("str w0, [x1]");
}

    

// void set_timer_interrupt(unsigned long seconds){

    

//     unsigned long cntfrq;
//     asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq));
//     // 计算过期时间
//     unsigned long tval = cntfrq * seconds;
//     // 启用定时器并设置时间
//     asm volatile ("msr cntp_tval_el0, %0" : : "r" (tval));
//     // 启用定时器，假设你的系统需要这么做
// }